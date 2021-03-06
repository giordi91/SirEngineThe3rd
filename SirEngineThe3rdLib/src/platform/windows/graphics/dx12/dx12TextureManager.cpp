#include "platform/windows/graphics/dx12/dx12TextureManager.h"

#include <DXTK12/DDSTextureLoader.h>
#include <platform/windows/graphics/dx12/dx12SwapChain.h>

#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "nlohmann/json.hpp"

namespace SirEngine::dx12 {

static const std::string TEXTURE_CUBE_KEY = "cube";
static const std::string TEXTURE_FORMAT_KEY = "format";
static const std::string TEXTURE_GAMMA_KEY = "gamma";
static const std::string TEXTURE_HAS_MIPS_KEY = "hasMips";
static const std::string TEXTURE_PATH_KEY = "path";
static const std::string DEFAULT_STRING = "";
static const int DEFAULT_INT = 0;
static const bool DEFAULT_BOOL = false;
static const char *WHITE_TEXTURE_PATH =
    "../data/processed/textures/white.texture";

static std::unordered_map<RenderTargetFormat, DXGI_FORMAT>
    RENDER_TARGET_FORMAT_TO_DXGI{
        {RenderTargetFormat::RGBA32, DXGI_FORMAT_R8G8B8A8_UNORM},
        {RenderTargetFormat::R11G11B10_FLOAT, DXGI_FORMAT_R11G11B10_FLOAT},
        {RenderTargetFormat::R11G11B10_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM},
        {RenderTargetFormat::R16G16B16A16_FLOAT,
         DXGI_FORMAT_R16G16B16A16_FLOAT},
        {RenderTargetFormat::BC1_UNORM, DXGI_FORMAT_BC1_UNORM},
        {RenderTargetFormat::DEPTH_F32_S8, DXGI_FORMAT_D32_FLOAT_S8X24_UINT},
        {RenderTargetFormat::BGRA32, DXGI_FORMAT_B8G8R8A8_UNORM}};

static const std::unordered_map<RESOURCE_STATE, D3D12_RESOURCE_STATES>
    RESOURCE_STATE_TO_DX_STATE = {
        {RESOURCE_STATE::GENERIC, D3D12_RESOURCE_STATE_COMMON},
        {RESOURCE_STATE::RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET},
        {RESOURCE_STATE::SHADER_READ_RESOURCE,
         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE},
        {RESOURCE_STATE::RANDOM_WRITE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS},
        {RESOURCE_STATE::DEPTH_RENDER_TARGET,
         D3D12_RESOURCE_STATE_DEPTH_WRITE}};

Dx12TextureManager::~Dx12TextureManager() {
  // assert(m_texturePool.assertEverythingDealloc());
}

TextureHandle Dx12TextureManager::loadTexture(const char *path,
                                              const bool cubeMap) {
  const bool res = fileExists(path);
  assert(res);

  nlohmann::json jobj;
  SirEngine::getJsonObj(path, jobj);
  const bool isCube = getValueIfInJson(jobj, TEXTURE_CUBE_KEY, DEFAULT_BOOL);
  const bool isGamma = getValueIfInJson(jobj, TEXTURE_GAMMA_KEY, DEFAULT_BOOL);
  const bool hasMips =
      getValueIfInJson(jobj, TEXTURE_HAS_MIPS_KEY, DEFAULT_BOOL);
  const std::string texturePath =
      getValueIfInJson(jobj, TEXTURE_PATH_KEY, DEFAULT_STRING);
  const int formatInt = getValueIfInJson(jobj, TEXTURE_FORMAT_KEY, DEFAULT_INT);

  const std::string name = getFileName(texturePath);

  TextureHandle handle{};
  if (!m_nameToHandle.get(name.c_str(), handle)) {
    // const auto found = m_nameToHandle.find(name);
    // if (found == m_nameToHandle.end()) {
    const std::string extension = getFileExtension(texturePath);
    if (extension != ".dds") {
      assert(0 && "can only load dds");
    }

    uint32_t index;
    TextureData &data = m_texturePool.getFreeMemoryData(index);
    const std::string paths(texturePath);
    const std::wstring pathws(paths.begin(), paths.end());
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;

    batch.Begin();
    DirectX::DDS_LOADER_FLAGS loadF =
        isGamma ? DirectX::DDS_LOADER_FORCE_SRGB : DirectX::DDS_LOADER_DEFAULT;

    DirectX::CreateDDSTextureFromFileEx(dx12::DEVICE, batch, pathws.c_str(), 0,
                                        D3D12_RESOURCE_FLAG_NONE, loadF,
                                        &data.resource);
    batch.End(dx12::GLOBAL_COMMAND_QUEUE);

    // data is now loaded need to create handle etc
    handle = {(MAGIC_NUMBER_COUNTER << 16) | index};

    data.magicNumber = MAGIC_NUMBER_COUNTER;
    data.format = data.resource->GetDesc().Format;
    data.state = D3D12_RESOURCE_STATE_COMMON;

    ++MAGIC_NUMBER_COUNTER;

    m_nameToHandle.insert(name.c_str(), handle);

    if (!cubeMap) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(data.srv, data.resource,
                                                        data.format);
    } else {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTextureCubeSRV(
          data.srv, data.resource, data.format);
    }

    return handle;
  }
  SE_CORE_INFO("Texture already loaded, returning handle: {0}", name);
  return handle;
}

D3D12_RESOURCE_STATES convertState(const RESOURCE_STATE state) {
  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old
  // layout before it will be transitioned to the new layout
  switch (state) {
    case RESOURCE_STATE::SHADER_READ_RESOURCE:
      return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case RESOURCE_STATE::GENERIC:
      return D3D12_RESOURCE_STATE_COMMON;
    case RESOURCE_STATE::RENDER_TARGET:
      return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case RESOURCE_STATE::DEPTH_RENDER_TARGET:
      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case RESOURCE_STATE::RANDOM_WRITE:
      return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    default:
      assert(0 && "unsupported texture resource state case");
      return D3D12_RESOURCE_STATE_COMMON;
  }
}

void Dx12TextureManager::transitionTexture(CommandBufferHandle commandBuffer,
                                           TextureHandle texHandle,
                                           RESOURCE_STATE currState,
                                           RESOURCE_STATE newState) {
  assertMagicNumber(texHandle);
  const uint32_t index = getIndexFromHandle(texHandle);
  TextureData &data = m_texturePool[index];
  D3D12_RESOURCE_BARRIER barriers[1];

  barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
      data.resource, convertState(currState), convertState(newState));
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->ResourceBarrier(1, barriers);
}

TextureHandle Dx12TextureManager::initializeFromResourceDx12(
    ID3D12Resource *resource, const char *name,
    const D3D12_RESOURCE_STATES state) {
  // since we are passing one resource, by definition the resource is static
  // data is now loaded need to create handle etc
  uint32_t index;
  TextureData &data = m_texturePool.getFreeMemoryData(index);
  const TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

  data.resource = resource;
  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = state;
  data.flags = TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET;

  ++MAGIC_NUMBER_COUNTER;

  dx12::createRTVSRV(dx12::GLOBAL_RTV_HEAP,
                     m_texturePool.getConstRef(index).resource, data.rtsrv);

  m_nameToHandle.insert(name, handle);
  return handle;
}

void Dx12TextureManager::bindBackBuffer() const {
  const TextureHandle destination =
      dx12::SWAP_CHAIN->currentBackBufferTexture();
  D3D12_RESOURCE_BARRIER barriers[2];
  int counter = 0;
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      destination, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  auto back = dx12::SWAP_CHAIN->currentBackBufferView();
  commandList->OMSetRenderTargets(1, &back, true, nullptr);
}

DescriptorPair Dx12TextureManager::getSrvStencilDx12(
    const TextureHandle handle) {
  {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    TextureData &data = m_texturePool[index];
    if (data.srv.cpuHandle.ptr == 0) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(
          data.dsvStencil, data.resource, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);

      dx12::createDSV(dx12::GLOBAL_DSV_HEAP, m_texturePool[index].resource,
                      data.dsvStencil, DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
    }
    assert(data.srv.type == DescriptorType::SRV);
    return m_texturePool.getConstRef(index).srv;
  }
}

D3D12_RESOURCE_STATES Dx12TextureManager::toDx12ResourceState(
    const RESOURCE_STATE state) {
  auto found = RESOURCE_STATE_TO_DX_STATE.find(state);
  if (found != RESOURCE_STATE_TO_DX_STATE.end()) {
    return found->second;
  }
  assert(
      0 &&
      "Could not find requested resource state for conversion to dx12 state");
  return D3D12_RESOURCE_STATE_COMMON;
}

void Dx12TextureManager::free(const TextureHandle handle) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  TextureData &data = m_texturePool[index];

  // check type
  if ((data.flags & TEXTURE_ALLOCATION_FLAG_BITS::DEPTH_TEXTURE) > 0) {
    if (data.srv.cpuHandle.ptr != 0) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.srv);
    }
    if (data.rtsrv.cpuHandle.ptr != 0) {
      dx12::GLOBAL_DSV_HEAP->freeDescriptor(data.rtsrv);
    }
    if (data.dsvStencil.cpuHandle.ptr != 0) {
      dx12::GLOBAL_DSV_HEAP->freeDescriptor(data.dsvStencil);
    }
    if (data.uav.cpuHandle.ptr != 0) {
      assert(0 && "not supported yet check if is correct");
      dx12::GLOBAL_DSV_HEAP->freeDescriptor(data.uav);
    }
  } else if ((data.flags & TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET) > 0) {
    if (data.srv.cpuHandle.ptr != 0) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.srv);
    }
    if (data.rtsrv.cpuHandle.ptr != 0) {
      dx12::GLOBAL_RTV_HEAP->freeDescriptor(data.rtsrv);
    }

    if (data.uav.cpuHandle.ptr != 0) {
      assert(0 && "not supported yet check if is correct");
      dx12::GLOBAL_RTV_HEAP->freeDescriptor(data.uav);
    }
  } else {
    if (data.srv.cpuHandle.ptr != 0) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.srv);
    }
    if (data.uav.cpuHandle.ptr != 0) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.uav);
    }
  }

  // releasing the texture;
  data.resource->Release();
  // invalidating magic number
  data.magicNumber = 0;
  // adding the index to the free list
  m_texturePool.free(index);
}

inline DXGI_FORMAT convertToDXGIFormat(const RenderTargetFormat format) {
  auto found = RENDER_TARGET_FORMAT_TO_DXGI.find(format);
  if (found != RENDER_TARGET_FORMAT_TO_DXGI.end()) {
    return found->second;
  }
  assert(0 && "Could not convert render target format to DXGI");
  return DXGI_FORMAT_UNKNOWN;
}

TextureHandle Dx12TextureManager::allocateTexture(
    const uint32_t width, const uint32_t height,
    const RenderTargetFormat format, const char *name,
    const uint32_t allocFlags, RESOURCE_STATE finalState) {
  // convert SirEngine format to dx12 format
  DXGI_FORMAT actualFormat = convertToDXGIFormat(format);

  uint32_t index;
  TextureData &data = m_texturePool.getFreeMemoryData(index);
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
  bool allowWrite =
      allocFlags &
      static_cast<uint32_t>(TEXTURE_ALLOCATION_FLAG_BITS::ALLOW_RANDOM_WRITE);
  bool isRenderTexture =
      allocFlags &
      static_cast<uint32_t>(TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET);
  bool isDepth = allocFlags & static_cast<uint32_t>(
                                  TEXTURE_ALLOCATION_FLAG_BITS::DEPTH_TEXTURE);
  assert(!(isRenderTexture && isDepth) &&
         "Cannot be both render texture and depth");

  flags |= isRenderTexture ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                           : D3D12_RESOURCE_FLAG_NONE;
  flags |= allowWrite ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                      : D3D12_RESOURCE_FLAG_NONE;
  flags |= isDepth ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
                   : D3D12_RESOURCE_FLAG_NONE;

  auto description = CD3DX12_RESOURCE_DESC::Tex2D(actualFormat, width, height,
                                                  1, 1, 1, 0, flags);

  D3D12_CLEAR_VALUE clear;
  if (!isDepth) {
    clear.Color[0] = 0.0f;
    clear.Color[1] = 0.0f;
    clear.Color[2] = 0.0f;
    clear.Color[3] = 1.0f;
  } else {
    clear.DepthStencil.Depth = 0.0f;
    clear.DepthStencil.Stencil = 0;
  }
  clear.Format = actualFormat;
  D3D12_RESOURCE_STATES state = convertState(finalState);
  auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  HRESULT hr = dx12::DEVICE->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &description, state, &clear,
      IID_PPV_ARGS(&data.resource));
  assert(SUCCEEDED(hr));

  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = state;
  // TODO change texture allocation flags similar to vulkan, with bits and
  // flags combination
  data.flags = allocFlags;

  TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  ++MAGIC_NUMBER_COUNTER;

  // here it diverges depending on the type of resource
  if (isDepth) {
    dx12::createDSV(dx12::GLOBAL_DSV_HEAP, m_texturePool[index].resource,
                    data.rtsrv, DXGI_FORMAT_D32_FLOAT_S8X24_UINT);

    dx12::createDSV(
        dx12::GLOBAL_DSV_HEAP, m_texturePool[index].resource, data.dsvStencil,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        (D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL));

    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(
        data.srv, data.resource, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);

  } else {
    createRTVSRV(dx12::GLOBAL_RTV_HEAP, data.resource, data.rtsrv);
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(data.srv, data.resource,
                                                      data.format);
    if (allowWrite) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DUAV(data.uav, data.resource,
                                                        data.format);
    }
  }
  data.resource->SetName(persistentConvertWide(name));

  m_nameToHandle.insert(name, handle);
  return handle;
}

void Dx12TextureManager::clearDepth(const TextureHandle depth,
                                    const float depthValue,
                                    const float stencilValue) const {
  assertMagicNumber(depth);
  const uint32_t index = getIndexFromHandle(depth);
  const TextureData &data = m_texturePool.getConstRef(index);
  assert((data.flags & TEXTURE_ALLOCATION_FLAG_BITS::DEPTH_TEXTURE) > 0);

  CURRENT_FRAME_RESOURCE->fc.commandList->ClearDepthStencilView(
      data.rtsrv.cpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
      depthValue, static_cast<uint8_t>(stencilValue), 0, nullptr);
}

void Dx12TextureManager::clearRT(const TextureHandle handle,
                                 const float color[4]) const {
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const TextureData &data = m_texturePool.getConstRef(index);
  assert((data.flags & TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET) > 0);
  // Clear the back buffer and depth buffer.
  CURRENT_FRAME_RESOURCE->fc.commandList->ClearRenderTargetView(
      data.rtsrv.cpuHandle, color, 0, nullptr);
}

void Dx12TextureManager::initialize() {
  m_whiteTexture = loadTexture(WHITE_TEXTURE_PATH, false);
}

void Dx12TextureManager::cleanup() {
  assertMagicNumber(m_whiteTexture);
  const uint32_t index = getIndexFromHandle(m_whiteTexture);
  const TextureData &data = m_texturePool.getConstRef(index);
  // TODO check what other view/resources need to be released
  data.resource->Release();
}
}  // namespace SirEngine::dx12
