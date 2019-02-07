#include "platform/windows/graphics/dx12/DX12.h"
#include "SirEngine/Window.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/adapter.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/swapChain.h"
#include "platform/windows/graphics/dx12/textureManager.h"
#include "platform/windows/graphics/dx12/meshManager.h"
#include "SirEngine/identityManager.h"

namespace SirEngine {
namespace dx12 {

#if DXR_ENABLED
ID3D12Device5 *DEVICE;
#else
ID3D12Device4 *DEVICE;
#endif
ID3D12Debug *DEBUG_CONTROLLER = nullptr;
IDXGIFactory6 *DXGI_FACTORY = nullptr;
Adapter *ADAPTER = nullptr;
UINT64 CURRENT_FENCE = 0;
UINT64 CURRENT_FRAME = 0;
DescriptorHeap *GLOBAL_CBV_SRV_UAV_HEAP = nullptr;
DescriptorHeap *GLOBAL_RTV_HEAP = nullptr;
DescriptorHeap *GLOBAL_DSV_HEAP = nullptr;
ID3D12CommandQueue *GLOBAL_COMMAND_QUEUE = nullptr;
ID3D12Fence *GLOBAL_FENCE = nullptr;
SwapChain *SWAP_CHAIN = nullptr;
FrameResource FRAME_RESOURCES[FRAME_BUFFERS_COUNT];
FrameResource *CURRENT_FRAME_RESOURCE = nullptr;
TextureManager *TEXTURE_MANAGER = nullptr;
MeshManager *MESH_MANAGER= nullptr;
IdentityManager * IDENTITY_MANAGER =nullptr;

bool createFrameCommand(FrameCommand *fc) {

  auto result = DEVICE->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fc->commandAllocator));
  if (FAILED(result)) {
    return false;
  }

  result = DEVICE->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     fc->commandAllocator, nullptr,
                                     IID_PPV_ARGS(&fc->commandList));
  if (FAILED(result)) {
    return false;
  }
  fc->commandList->Close();
  fc->isListOpen = false;
  return true;
}

bool initializeGraphicsDx12(Window *wnd, uint32_t width, uint32_t height) {

  // lets enable debug layer if needed
  //#if defined(DEBUG) || defined(_DEBUG)
  {
    HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&DEBUG_CONTROLLER));
    if (FAILED(result)) {
      return false;
    }
    DEBUG_CONTROLLER->EnableDebugLayer();

    //    ID3D12Debug1* debug1;
    //    m_debugController->QueryInterface(IID_PPV_ARGS(&debug1));
    //    debug1->SetEnableGPUBasedValidation(true);
  }
  //#endif

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
    DX12Handles::device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
                                             &opts5, sizeof(opts5));
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

  //initialize the managers
  TEXTURE_MANAGER = new TextureManager();
  MESH_MANAGER = new MeshManager();
  IDENTITY_MANAGER = new IdentityManager();
  IDENTITY_MANAGER->initialize();

  // init swap chain
  auto *windowWnd = static_cast<HWND>(wnd->getNativeWindow());

  dx12::SWAP_CHAIN = new dx12::SwapChain();
  dx12::SWAP_CHAIN->initialize(windowWnd, width, height);
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  dx12::SWAP_CHAIN->resize(&dx12::CURRENT_FRAME_RESOURCE->fc, width, height);

  return true;
}

bool shutdownGraphicsDx12()
{
	flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);

	//free the swapchain
	delete SWAP_CHAIN;

	//deleting the managers
	delete MESH_MANAGER;
	delete TEXTURE_MANAGER;
	return true;
}

bool stopGraphicsDx12()
{
	flushCommandQueue(GLOBAL_COMMAND_QUEUE);
	return true;
}
} // namespace dx12
} // namespace SirEngine
