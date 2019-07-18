#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include <DXTK12/DDSTextureLoader.h>
#include <platform/windows/graphics/dx12/swapChain.h>
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

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
        {RenderTargetFormat::BC1_UNORM, DXGI_FORMAT_BC1_UNORM}};

TextureManagerDx12::~TextureManagerDx12() {
  // assert(m_texturePool.assertEverythingDealloc());
}

void TextureManagerDx12::loadLegacy(const std::string &) {
  assert(0 && "legacy not yet supported");
}

TextureHandle TextureManagerDx12::loadTexture(const char *path,
                                              const bool cubeMap) {
  const bool res = fileExists(path);
  assert(res);

  const auto jobj = getJsonObj(path);
  const bool isCube = getValueIfInJson(jobj, TEXTURE_CUBE_KEY, DEFAULT_BOOL);
  const bool isGamma = getValueIfInJson(jobj, TEXTURE_GAMMA_KEY, DEFAULT_BOOL);
  const bool hasMips =
      getValueIfInJson(jobj, TEXTURE_HAS_MIPS_KEY, DEFAULT_BOOL);
  const std::string texturePath =
      getValueIfInJson(jobj, TEXTURE_PATH_KEY, DEFAULT_STRING);
  const int formatInt = getValueIfInJson(jobj, TEXTURE_FORMAT_KEY, DEFAULT_INT);

  const std::string name = getFileName(texturePath);

  const auto found = m_nameToHandle.find(name);
  if (found == m_nameToHandle.end()) {
    const std::string extension = getFileExtension(texturePath);
    if (extension != ".dds") {
      loadLegacy(path);
      // not supported
      assert(0);
    }

    uint32_t index;
    TextureData &data = m_texturePool.getFreeMemoryData(index);
    const std::string paths(texturePath);
    const std::wstring pathws(paths.begin(), paths.end());
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;

    batch.Begin();
    // DirectX::CreateDDSTextureFromFile(dx12::DEVICE, batch, pathws.c_str(),
    //                                  &data.resource, false);
    DirectX::DDS_LOADER_FLAGS loadF =
        isGamma ? DirectX::DDS_LOADER_FORCE_SRGB : DirectX::DDS_LOADER_DEFAULT;

    DirectX::CreateDDSTextureFromFileEx(dx12::DEVICE, batch, pathws.c_str(), 0,
                                        D3D12_RESOURCE_FLAG_NONE, loadF,
                                        &data.resource);
    batch.End(dx12::GLOBAL_COMMAND_QUEUE);

    // data is now loaded need to create handle etc
    const TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

    data.magicNumber = MAGIC_NUMBER_COUNTER;
    data.format = data.resource->GetDesc().Format;
    data.state = D3D12_RESOURCE_STATE_COMMON;

    ++MAGIC_NUMBER_COUNTER;

    m_nameToHandle[name] = handle;

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
  return found->second;
}

TextureHandle TextureManagerDx12::initializeFromResourceDx12(
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
  data.flags = TextureFlags::RT;

  ++MAGIC_NUMBER_COUNTER;

  dx12::createRTVSRV(dx12::GLOBAL_RTV_HEAP,
                     m_texturePool.getConstRef(index).resource, data.rtsrv);

  m_nameToHandle[name] = handle;
  return handle;
}

TextureHandle TextureManagerDx12::createDepthTexture(
    const char *name, const uint32_t width, const uint32_t height,
    const D3D12_RESOURCE_STATES state) {
  const bool m_4xMsaaState = false;

  // Create the depth/stencil buffer and view.
  D3D12_RESOURCE_DESC depthStencilDesc;
  depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depthStencilDesc.Alignment = 0;
  depthStencilDesc.Width = width;
  depthStencilDesc.Height = height;
  depthStencilDesc.DepthOrArraySize = 1;
  depthStencilDesc.MipLevels = 1;

  // Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to
  // read from the depth buffer.  Therefore, because we need to create two views
  // to the same resource:
  //   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
  //   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
  // we need to create the depth buffer resource with a typeless format.
  depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

  // Check 4X MSAA quality support for our back buffer format.
  // All Direct3D 11 capable devices support 4X MSAA for all render
  // target formats, so we only need to check quality support.
  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
  msQualityLevels.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
  msQualityLevels.SampleCount = 4;
  msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
  msQualityLevels.NumQualityLevels = 0;
  HRESULT res =
      DEVICE->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                  &msQualityLevels, sizeof(msQualityLevels));
  assert(SUCCEEDED(res));
  const UINT m_msaaQuality = msQualityLevels.NumQualityLevels;

  depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
  depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_msaaQuality - 1) : 0;
  depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  uint32_t index;
  TextureData &data = m_texturePool.getFreeMemoryData(index);
  D3D12_CLEAR_VALUE optClear;
  optClear.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
  optClear.DepthStencil.Depth = 0.0f;
  optClear.DepthStencil.Stencil = 0;
  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  res = DEVICE->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                                        &depthStencilDesc, state, &optClear,
                                        IID_PPV_ARGS(&data.resource));
  assert(SUCCEEDED(res));

  data.flags = TextureFlags::DEPTH;
  // we have the texture, we set the flag, we now need to create handle etc
  // data is now loaded need to create handle etc
  const TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = state;

  dx12::createDSV(dx12::GLOBAL_DSV_HEAP, m_texturePool[index].resource,
                  data.rtsrv, DXGI_FORMAT_D32_FLOAT_S8X24_UINT);

  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(
      data.srv, data.resource, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;
  return handle;
}

void TextureManagerDx12::free(const TextureHandle handle) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  TextureData &data = m_texturePool[index];

  // check type
  if ((data.flags & TextureFlags::DEPTH) > 0) {
    if (data.srv.cpuHandle.ptr != 0) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.srv);
    }
    if (data.rtsrv.cpuHandle.ptr != 0) {
      dx12::GLOBAL_DSV_HEAP->freeDescriptor(data.rtsrv);
    }
    if (data.uav.cpuHandle.ptr != 0) {
      assert(0 && "not supported yet check if is correct");
      dx12::GLOBAL_DSV_HEAP->freeDescriptor(data.uav);
    }
  } else if ((data.flags & TextureFlags::RT) > 0) {
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

auto TextureManagerDx12::allocateRenderTexture(const uint32_t width,
                                               const uint32_t height,
                                               const RenderTargetFormat format,
                                               const char *name,
                                               bool allowWrite)
    -> TextureHandle {
  // convert SirEngine format to dx12 format
  DXGI_FORMAT actualFormat = convertToDXGIFormat(format);

  uint32_t index;
  TextureData &data = m_texturePool.getFreeMemoryData(index);
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  if (allowWrite) {
    flags = flags | (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  }
  auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(actualFormat, width, height, 1, 1,
                                              1, 0, flags);

  D3D12_CLEAR_VALUE clear;
  clear.Color[0] = 0.0f;
  clear.Color[1] = 0.0f;
  clear.Color[2] = 0.0f;
  clear.Color[3] = 1.0f;
  clear.Format = actualFormat;
  auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  HRESULT hr = dx12::DEVICE->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc,
      D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, IID_PPV_ARGS(&data.resource));
  assert(SUCCEEDED(hr));

  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = D3D12_RESOURCE_STATE_RENDER_TARGET;
  data.flags = TextureFlags::RT;

  TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

  ++MAGIC_NUMBER_COUNTER;

  createRTVSRV(dx12::GLOBAL_RTV_HEAP, data.resource, data.rtsrv);
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(data.srv, data.resource,
                                                    data.format);
  if (allowWrite) {
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DUAV(data.uav, data.resource,
                                                      data.format);
  }

  // convert to wstring
  const std::string sname(name);
  const std::wstring wname(sname.begin(), sname.end());
  data.resource->SetName(wname.c_str());

  m_nameToHandle[name] = handle;
  return handle;
}

TextureHandle TextureManagerDx12::allocateTexture(
    const uint32_t width, const uint32_t height,
    const RenderTargetFormat format, const char *name, const bool mips,
    const bool allowWrite) {
  // convert SirEngine format to dx12 format
  const DXGI_FORMAT actualFormat = convertToDXGIFormat(format);

  uint32_t index;
  TextureData &data = m_texturePool.getFreeMemoryData(index);
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  if (allowWrite) {
    flags = flags | (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  }

  const uint16_t mipsLevel = mips ? static_cast<uint16_t>(std::log2(width)) : 1;
  auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(actualFormat, width, height, 1,
                                              mipsLevel, 1, 0, flags);

  const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
  auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  HRESULT hr = dx12::DEVICE->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, state, nullptr,
      IID_PPV_ARGS(&data.resource));
  assert(SUCCEEDED(hr));

  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = state;
  data.flags = static_cast<TextureFlags>(0);

  const TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

  ++MAGIC_NUMBER_COUNTER;

  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(data.srv, data.resource,
                                                    data.format);
  if (allowWrite) {
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DUAV(data.uav, data.resource,
                                                      data.format);
  }

  // convert to wstring
  const std::string sname(name);
  const std::wstring wname(sname.begin(), sname.end());
  data.resource->SetName(wname.c_str());

  m_nameToHandle[name] = handle;
  return handle;
}

void TextureManagerDx12::bindRenderTarget(const TextureHandle handle,
                                          const TextureHandle depth) {
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const TextureData &data = m_texturePool.getConstRef(index);
  assert((data.flags & TextureFlags::RT) > 0);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_CPU_DESCRIPTOR_HANDLE handles[1] = {data.rtsrv.cpuHandle};
  // TODO fix this, should not have a depth the swap chain??
  // auto backDepth = dx12::SWAP_CHAIN->getDepthCPUDescriptor();
  const D3D12_CPU_DESCRIPTOR_HANDLE *depthDesc = nullptr;
  if (depth.isHandleValid()) {
    assertMagicNumber(depth);
    const uint32_t depthIndex = getIndexFromHandle(depth);
    const TextureData &depthData = m_texturePool.getConstRef(depthIndex);
    assert((depthData.flags & TextureFlags::DEPTH) > 0);
    depthDesc = &(depthData.rtsrv.cpuHandle);
  }
  commandList->OMSetRenderTargets(1, handles, true, depthDesc);
}

void TextureManagerDx12::copyTexture(const TextureHandle source,
                                     const TextureHandle destination) {
  assertMagicNumber(source);
  assertMagicNumber(destination);

  const uint32_t sourceIdx = getIndexFromHandle(source);
  const uint32_t destIdx = getIndexFromHandle(destination);

  TextureData &sourceData = m_texturePool[sourceIdx];
  TextureData &destData = m_texturePool[destIdx];

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[2];

  int counter = 0;
  auto state = sourceData.state;
  if (sourceData.state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
    barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
        sourceData.resource, state, D3D12_RESOURCE_STATE_COPY_SOURCE);
    sourceData.state = D3D12_RESOURCE_STATE_COPY_SOURCE;
    ++counter;
  }
  state = destData.state;
  if (destData.state != D3D12_RESOURCE_STATE_COPY_DEST) {
    barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
        destData.resource, state, D3D12_RESOURCE_STATE_COPY_DEST);
    destData.state = D3D12_RESOURCE_STATE_COPY_DEST;
    ++counter;
  }
  if (counter > 0) {
    commandList->ResourceBarrier(counter, barriers);
  }
  commandList->CopyResource(destData.resource, sourceData.resource);
}

void TextureManagerDx12::bindBackBuffer(bool bindBackBufferDepth) {
  auto back = dx12::SWAP_CHAIN->currentBackBufferView();
  auto depth = dx12::SWAP_CHAIN->getDepthCPUDescriptor();
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->OMSetRenderTargets(1, &back, true,
                                  bindBackBufferDepth ? &depth : nullptr);
  ;
}

void TextureManagerDx12::clearDepth(const TextureHandle depth,
                                    const float value) {
  assertMagicNumber(depth);
  const uint32_t index = getIndexFromHandle(depth);
  const TextureData &data = m_texturePool.getConstRef(index);
  assert((data.flags & TextureFlags::DEPTH) > 0);

  CURRENT_FRAME_RESOURCE->fc.commandList->ClearDepthStencilView(
      data.rtsrv.cpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
      value, 0, 0, nullptr);
}

void TextureManagerDx12::clearRT(const TextureHandle handle,
                                 const float color[4]) {
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const TextureData &data = m_texturePool.getConstRef(index);
  assert((data.flags & TextureFlags::RT) > 0);
  // Clear the back buffer and depth buffer.
  CURRENT_FRAME_RESOURCE->fc.commandList->ClearRenderTargetView(
      data.rtsrv.cpuHandle, color, 0, nullptr);
}

void TextureManagerDx12::initialize() {
  m_whiteTexture = loadTexture(WHITE_TEXTURE_PATH, false);
}
}  // namespace SirEngine::dx12
