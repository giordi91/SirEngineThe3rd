#include "platform/windows/graphics/dx12/DX12.h"
#include "SirEngine/Window.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/identityManager.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/adapter.h"
#include "platform/windows/graphics/dx12/bufferManagerDx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/meshManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/shaderLayout.h"
#include "platform/windows/graphics/dx12/shaderManager.h"
#include "platform/windows/graphics/dx12/swapChain.h"

namespace SirEngine::dx12 {

D3D12DeviceType *DEVICE;
ID3D12Debug *DEBUG_CONTROLLER = nullptr;
IDXGIFactory6 *DXGI_FACTORY = nullptr;
Adapter *ADAPTER = nullptr;
UINT64 CURRENT_FENCE = 0;
DescriptorHeap *GLOBAL_CBV_SRV_UAV_HEAP = nullptr;
DescriptorHeap *GLOBAL_RTV_HEAP = nullptr;
DescriptorHeap *GLOBAL_DSV_HEAP = nullptr;
ID3D12CommandQueue *GLOBAL_COMMAND_QUEUE = nullptr;
ID3D12Fence *GLOBAL_FENCE = nullptr;
SwapChain *SWAP_CHAIN = nullptr;
FrameResource FRAME_RESOURCES[FRAME_BUFFERS_COUNT];
FrameResource *CURRENT_FRAME_RESOURCE = nullptr;
TextureManagerDx12 *TEXTURE_MANAGER = nullptr;
MeshManager *MESH_MANAGER = nullptr;
IdentityManager *IDENTITY_MANAGER = nullptr;
MaterialManager *MATERIAL_MANAGER = nullptr;
Graph *RENDERING_GRAPH = nullptr;
ConstantBufferManagerDx12 *CONSTANT_BUFFER_MANAGER = nullptr;
ShaderManager *SHADER_MANAGER = nullptr;
PSOManager *PSO_MANAGER = nullptr;
RootSignatureManager *ROOT_SIGNATURE_MANAGER = nullptr;
ShadersLayoutRegistry *SHADER_LAYOUT_REGISTRY = nullptr;
BufferManagerDx12 *BUFFER_MANAGER = nullptr;

void createFrameCommand(FrameCommand *fc) {

  auto result = DEVICE->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fc->commandAllocator));
  assert(SUCCEEDED(result));

  result = DEVICE->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     fc->commandAllocator, nullptr,
                                     IID_PPV_ARGS(&fc->commandList));
  assert(SUCCEEDED(result));
  fc->commandList->Close();
  fc->isListOpen = false;
}

bool initializeGraphicsDx12(Window *wnd, uint32_t width, uint32_t height) {

// lets enable debug layer if needed
#if defined(DEBUG) || defined(_DEBUG)
  {
    HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&DEBUG_CONTROLLER));
    if (FAILED(result)) {
      return false;
    }
    DEBUG_CONTROLLER->EnableDebugLayer();
    // ID3D12Debug1 *debug1;
    // DEBUG_CONTROLLER->QueryInterface(IID_PPV_ARGS(&debug1));
    // debug1->SetEnableGPUBasedValidation(true);
  }
#endif

  HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&DXGI_FACTORY));
  if (FAILED(result)) {
    return false;
  }

  ADAPTER = new Adapter();
#if DXR_ENABLED
  ADAPTER->setFeture(AdapterFeature::DXR);
  ADAPTER->setVendor(AdapterVendor::NVIDIA);
#else
  ADAPTER->setFeture(AdapterFeature::ANY);
  ADAPTER->setVendor(AdapterVendor::ANY);
#endif
  bool found = ADAPTER->findBestAdapter(DXGI_FACTORY);
  assert(found && "could not find adapter matching features");

  // log the adapter used
  auto *adapter = ADAPTER->getAdapter();
  DXGI_ADAPTER_DESC desc;
  HRESULT adapterDescRes = SUCCEEDED(adapter->GetDesc(&desc));
  assert(SUCCEEDED(adapterDescRes));
  char t[128];
  size_t converted = 0;
  wcstombs_s(&converted, t, desc.Description, 128);
  SE_CORE_INFO(t);

  result = D3D12CreateDevice(ADAPTER->getAdapter(), D3D_FEATURE_LEVEL_12_1,
                             IID_PPV_ARGS(&DEVICE));
  if (FAILED(result)) {
    SE_CORE_ERROR("Could not create device with requested features");
    // falling back to WARP device
    IDXGIAdapter *warpAdapter;
    result = DXGI_FACTORY->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
    if (FAILED(result)) {
      return false;
    }

    result = D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_12_1,
                               IID_PPV_ARGS(&DEVICE));
    if (FAILED(result)) {
      return false;
    }
  }

  // Check the maximum feature level, and make sure it's above our minimum
  D3D_FEATURE_LEVEL featureLevelsArray[1];
  featureLevelsArray[0] = D3D_FEATURE_LEVEL_12_1;
  D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
  featureLevels.NumFeatureLevels = 1;
  featureLevels.pFeatureLevelsRequested = featureLevelsArray;
  HRESULT r = DEVICE->CheckFeatureSupport(
      D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
  assert(featureLevels.MaxSupportedFeatureLevel == D3D_FEATURE_LEVEL_12_1);
  assert(SUCCEEDED(r));

#if DXR_ENABLED
  if (ADAPTER->getFeature() == AdapterFeature::DXR) {
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 opts5 = {};
    dx12::DEVICE->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &opts5,
                                      sizeof(opts5));
    if (opts5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
      assert(0);
  }
#endif

  // creating the command queue
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  auto qresult = DEVICE->CreateCommandQueue(
      &queueDesc, IID_PPV_ARGS(&GLOBAL_COMMAND_QUEUE));
  if (FAILED(qresult)) {
    return false;
  }

  result = DEVICE->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                               IID_PPV_ARGS(&GLOBAL_FENCE));
  if (FAILED(result)) {
    return false;
  }

  // creating global heaps
  GLOBAL_CBV_SRV_UAV_HEAP = new DescriptorHeap();
  GLOBAL_CBV_SRV_UAV_HEAP->initializeAsCBVSRVUAV(1000);

  GLOBAL_RTV_HEAP = new DescriptorHeap();
  GLOBAL_RTV_HEAP->initializeAsRTV(20);

  GLOBAL_DSV_HEAP = new DescriptorHeap();
  GLOBAL_DSV_HEAP->initializeAsDSV(20);

  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
    createFrameCommand(&FRAME_RESOURCES[i].fc);
  }

  CURRENT_FRAME_RESOURCE = &FRAME_RESOURCES[0];

  // initialize the managers
  // TODO add initialize to all managers for consistency and symmetry
  IDENTITY_MANAGER = new IdentityManager();
  IDENTITY_MANAGER->initialize();
  CONSTANT_BUFFER_MANAGER = new ConstantBufferManagerDx12();
  CONSTANT_BUFFER_MANAGER->initialize();

  BUFFER_MANAGER = new BufferManagerDx12();
  BUFFER_MANAGER->initialize();

  globals::CONSTANT_BUFFER_MANAGER = CONSTANT_BUFFER_MANAGER;
  globals::BUFFER_MANAGER = BUFFER_MANAGER;
  TEXTURE_MANAGER = new TextureManagerDx12();
  globals::TEXTURE_MANAGER = TEXTURE_MANAGER;
  MESH_MANAGER = new MeshManager();
  MATERIAL_MANAGER = new MaterialManager();
  MATERIAL_MANAGER->initialize();
  globals::ASSET_MANAGER = new AssetManager();
  globals::ASSET_MANAGER->initialize();
  globals::RENDERING_CONTEX = new RenderingContext();
  globals::RENDERING_CONTEX->initialize();

  SHADER_MANAGER = new ShaderManager();
  SHADER_MANAGER->init();
  SHADER_MANAGER->loadShadersInFolder(
      (globals::DATA_SOURCE_PATH + "/processed/shaders/rasterization").c_str());
  SHADER_MANAGER->loadShadersInFolder(
      (globals::DATA_SOURCE_PATH + "/processed/shaders/compute").c_str());

  ROOT_SIGNATURE_MANAGER = new RootSignatureManager();
  ROOT_SIGNATURE_MANAGER->loadSingaturesInFolder(
      (globals::DATA_SOURCE_PATH + "/processed/rs").c_str());

  SHADER_LAYOUT_REGISTRY = new dx12::ShadersLayoutRegistry();

  PSO_MANAGER = new PSOManager();
  PSO_MANAGER->init(dx12::DEVICE, SHADER_LAYOUT_REGISTRY,
                    ROOT_SIGNATURE_MANAGER, dx12::SHADER_MANAGER);
  PSO_MANAGER->loadPSOInFolder((globals::DATA_SOURCE_PATH + "/pso").c_str());

  globals::DEBUG_FRAME_DATA = new globals::DebugFrameData();

  bool isHeadless = (wnd == nullptr) | (width == 0) | (height == 0);

  if (!isHeadless) {
    // init swap chain
    auto *windowWnd = static_cast<HWND>(wnd->getNativeWindow());

    dx12::SWAP_CHAIN = new dx12::SwapChain();
    dx12::SWAP_CHAIN->initialize(windowWnd, width, height);
    dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
    dx12::SWAP_CHAIN->resize(&dx12::CURRENT_FRAME_RESOURCE->fc, width, height);
  } else {
    SE_CORE_INFO("Requested HEADLESS client, no swapchain is initialized");
  }

  return true;
}
void flushDx12() { flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE); }

bool beginHeadlessWorkDx12() {
  // here we need to check which frame resource we are going to use
  dx12::CURRENT_FRAME_RESOURCE = &dx12::FRAME_RESOURCES[globals::CURRENT_FRAME];

  // check if the resource has finished rendering if not we have to wait
  if (dx12::CURRENT_FRAME_RESOURCE->fence != 0 &&
      dx12::GLOBAL_FENCE->GetCompletedValue() <
          dx12::CURRENT_FRAME_RESOURCE->fence) {
    HANDLE eventHandle =
        CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    auto handleResult = dx12::GLOBAL_FENCE->SetEventOnCompletion(
        dx12::CURRENT_FRAME_RESOURCE->fence, eventHandle);
    assert(SUCCEEDED(handleResult));
    WaitForSingleObject(eventHandle, INFINITE);

    CloseHandle(eventHandle);
  }
  // at this point we know we are ready to go
  resetAllocatorAndList(&dx12::CURRENT_FRAME_RESOURCE->fc);
  // Indicate a state transition on the resource usage.
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;

  auto *heap = dx12::GLOBAL_CBV_SRV_UAV_HEAP->getResource();
  commandList->SetDescriptorHeaps(1, &heap);

  return true;
}

bool endHeadlessWorkDx12() {
  // Done recording commands.
  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE,
                           &dx12::CURRENT_FRAME_RESOURCE->fc);

  dx12::CURRENT_FRAME_RESOURCE->fence = ++dx12::CURRENT_FENCE;
  dx12::GLOBAL_COMMAND_QUEUE->Signal(dx12::GLOBAL_FENCE, dx12::CURRENT_FENCE);
  // bump the frame
  globals::CURRENT_FRAME = (globals::CURRENT_FRAME + 1) % FRAME_BUFFERS_COUNT;
  return true;
}

bool shutdownGraphicsDx12() {
  flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);

  // free the swapchain
  delete SWAP_CHAIN;

  // deleting the managers
  delete MESH_MANAGER;
  delete TEXTURE_MANAGER;
  return true;
}

bool stopGraphicsDx12() {
  flushCommandQueue(GLOBAL_COMMAND_QUEUE);
  return true;
}
bool newFrameDx12() {
  // here we need to check which frame resource we are going to use
  dx12::CURRENT_FRAME_RESOURCE = &dx12::FRAME_RESOURCES[globals::CURRENT_FRAME];

  // check if the resource has finished rendering if not we have to wait
  if (dx12::CURRENT_FRAME_RESOURCE->fence != 0 &&
      dx12::GLOBAL_FENCE->GetCompletedValue() <
          dx12::CURRENT_FRAME_RESOURCE->fence) {
    HANDLE eventHandle =
        CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    auto handleResult = dx12::GLOBAL_FENCE->SetEventOnCompletion(
        dx12::CURRENT_FRAME_RESOURCE->fence, eventHandle);
    assert(SUCCEEDED(handleResult));
    WaitForSingleObject(eventHandle, INFINITE);

    CloseHandle(eventHandle);
  }
  // at this point we know we are ready to go

  // Clear the back buffer and depth buffer.
  float gray[4] = {0.5f, 0.9f, 0.5f, 1.0f};
  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished
  // execution on the GPU.
  resetAllocatorAndList(&dx12::CURRENT_FRAME_RESOURCE->fc);
  // Indicate a state transition on the resource usage.
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
  D3D12_RESOURCE_BARRIER rtbarrier[1];

  TextureHandle backBufferH = dx12::SWAP_CHAIN->currentBackBufferTexture();
  int rtcounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      backBufferH, D3D12_RESOURCE_STATE_RENDER_TARGET, rtbarrier, 0);
  if (rtcounter != 0) {
    commandList->ResourceBarrier(rtcounter, rtbarrier);
  }

  // Set the viewport and scissor rect.  This needs to be reset whenever the
  // command list is reset.
  commandList->RSSetViewports(1, dx12::SWAP_CHAIN->getViewport());
  commandList->RSSetScissorRects(1, dx12::SWAP_CHAIN->getScissorRect());

  // Clear the back buffer and depth buffer.
  // commandList->ClearRenderTargetView(dx12::SWAP_CHAIN->currentBackBufferView(),
  //                                   gray, 0, nullptr);
  // dx12::SWAP_CHAIN->clearDepth();
  // dx12::SwapChain *swapChain = dx12::SWAP_CHAIN;
  //// Specify the buffers we are going to render to.
  // auto back = swapChain -> currentBackBufferView();
  // auto depth = swapChain -> getDepthCPUDescriptor();
  // commandList->OMSetRenderTargets(1, &back, true, &depth);
  auto *heap = dx12::GLOBAL_CBV_SRV_UAV_HEAP->getResource();
  commandList->SetDescriptorHeaps(1, &heap);

  return true;
}
bool dispatchFrameDx12() {
  D3D12_RESOURCE_BARRIER rtbarrier[1];
  // finally transition the resource to be present
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;

  TextureHandle backBufferH = dx12::SWAP_CHAIN->currentBackBufferTexture();
  int rtcounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      backBufferH, D3D12_RESOURCE_STATE_PRESENT, rtbarrier, 0);
  if (rtcounter != 0) {
    commandList->ResourceBarrier(rtcounter, rtbarrier);
  }

  // Done recording commands.
  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE,
                           &dx12::CURRENT_FRAME_RESOURCE->fc);

  dx12::CURRENT_FRAME_RESOURCE->fence = ++dx12::CURRENT_FENCE;
  dx12::GLOBAL_COMMAND_QUEUE->Signal(dx12::GLOBAL_FENCE, dx12::CURRENT_FENCE);
  // swap the back and front buffers
  dx12::SWAP_CHAIN->present();
  // bump the frame
  globals::CURRENT_FRAME = (globals::CURRENT_FRAME + 1) % FRAME_BUFFERS_COUNT;
  return true;
}
} // namespace SirEngine::dx12
