#include "platform/windows/graphics/dx12/DX12.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/adapter.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/swapChain.h"

namespace SirEngine {
namespace dx12 {
namespace DX12Handles {

#if DXR_ENABLED
ID3D12Device5 *device;
#else
ID3D12Device4 *device;
#endif
ID3D12Debug *debugController = nullptr;
IDXGIFactory6 *dxiFactory = nullptr;
Adapter *adapter = nullptr;
ID3D12CommandQueue *commandQueue = nullptr;
DescriptorHeap *globalCBVSRVUAVheap = nullptr;
DescriptorHeap *globalRTVheap = nullptr;
DescriptorHeap *globalDSVheap = nullptr;
UINT64 currentFence = 0;
UINT64 currentFrame = 0;
ID3D12Fence *fence = nullptr;
FrameCommand *frameCommand = nullptr;
SwapChain *swapChain = nullptr;
FrameResource frameResources[FRAME_BUFFERING_COUNT];
FrameResource *currenFrameResource = nullptr;
} // namespace DX12Handles

bool createFrameCommand(FrameCommand *fc) {

  auto result = DX12Handles::device->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fc->commandAllocator));
  if (FAILED(result)) {
    return false;
  }

  result = DX12Handles::device->CreateCommandList(
      0, D3D12_COMMAND_LIST_TYPE_DIRECT,
      DX12Handles::frameCommand->commandAllocator, nullptr,
      IID_PPV_ARGS(&fc->commandList));
  if (FAILED(result)) {
    return false;
  }
  fc->commandList->Close();
  fc->isListOpen = false;
}

bool initializeGraphics() {

  // lets enable debug layer if needed
  //#if defined(DEBUG) || defined(_DEBUG)
  {
    HRESULT result =
        D3D12GetDebugInterface(IID_PPV_ARGS(&DX12Handles::debugController));
    if (FAILED(result)) {
      return false;
    }
    DX12Handles::debugController->EnableDebugLayer();

    //    ID3D12Debug1* debug1;
    //    m_debugController->QueryInterface(IID_PPV_ARGS(&debug1));
    //    debug1->SetEnableGPUBasedValidation(true);
  }
  //#endif

  HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&DX12Handles::dxiFactory));
  if (FAILED(result)) {
    return false;
  }

  DX12Handles::adapter = new Adapter();
#if DXR_ENABLED
  DX12Handles::adapter->setFeture(AdapterFeature::DXR);
  DX12Handles::adapter->setVendor(AdapterVendor::NVIDIA);
#else
  DX12Handles::adapter->setFeture(AdapterFeature::ANY);
  DX12Handles::adapter->setVendor(AdapterVendor::ANY);
#endif
  bool found = DX12Handles::adapter->findBestAdapter(DX12Handles::dxiFactory);

  // log the adapter used
  auto *adapter = DX12Handles::adapter->getAdapter();
  DXGI_ADAPTER_DESC desc;
  HRESULT adapterDescRes = SUCCEEDED(adapter->GetDesc(&desc));
  char t[128];
  size_t converted = 0;
  size_t res = wcstombs_s(&converted, t, desc.Description, 128);
  SE_CORE_INFO(t);

  result = D3D12CreateDevice(DX12Handles::adapter->getAdapter(),
                             D3D_FEATURE_LEVEL_12_1,
                             IID_PPV_ARGS(&DX12Handles::device));
  if (FAILED(result)) {
    SE_CORE_ERROR("Could not create device with requested features");
    // falling back to WARP device
    IDXGIAdapter *warpAdapter;
    result =
        DX12Handles::dxiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
    if (FAILED(result)) {
      return false;
    }

    result = D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_12_1,
                               IID_PPV_ARGS(&DX12Handles::device));
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
  HRESULT r = DX12Handles::device->CheckFeatureSupport(
      D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
  assert(featureLevels.MaxSupportedFeatureLevel == D3D_FEATURE_LEVEL_12_1);

#if DXR_ENABLED
  if (DX12Handles::adapter->getFeature() == AdapterFeature::DXR) {
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
  auto qresult = DX12Handles::device->CreateCommandQueue(
      &queueDesc, IID_PPV_ARGS(&DX12Handles::commandQueue));
  if (FAILED(qresult)) {
    return false;
  }

  DX12Handles::frameCommand = new FrameCommand();
  createFrameCommand(DX12Handles::frameCommand);

  result = DX12Handles::device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                            IID_PPV_ARGS(&DX12Handles::fence));
  if (FAILED(result)) {
    return false;
  }

  // creating global heaps
  DX12Handles::globalCBVSRVUAVheap = new DescriptorHeap();
  DX12Handles::globalCBVSRVUAVheap->initializeAsCBVSRVUAV(1000);

  DX12Handles::globalRTVheap = new DescriptorHeap();
  DX12Handles::globalRTVheap->initializeAsRTV(20);

  DX12Handles::globalDSVheap = new DescriptorHeap();
  DX12Handles::globalDSVheap->initializeAsDSV(20);

  for (int i = 0; i < FRAME_BUFFERING_COUNT; ++i) {
    createFrameCommand(&DX12Handles::frameResources[i].fc);
  }

  DX12Handles::currenFrameResource = &DX12Handles::frameResources[0];
  return true;
}
} // namespace dx12
} // namespace SirEngine
