#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include "SirEngine/globals.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include <cassert>
#include "SirEngine/memory/hashMap.h"
#include "SirEngine/memory/resizableVector.h"

namespace SirEngine {
class IdentityManager;
class BaseWindow;
class AssetManager;
class Dx12MaterialManager;
class DependencyGraph;

namespace dx12 {
class Dx12TextureManager;
class Dx12MeshManager;
class Dx12Adapter;
class DescriptorHeap;
class Dx12SwapChain;
class Dx12ConstantBufferManager;
class ShaderManager;
class RootSignatureManager;
class ShadersLayoutRegistry;
class Dx12PSOManager;
class BufferManagerDx12;
class DebugRenderer;
class Dx12RenderingContext;

enum class DescriptorType {
  NONE = 0,
  CBV = 1,
  SRV = 2,
  UAV = 4,
  RTV = 8,
  DSV = 16
};

struct DescriptorPair {
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
#if SE_DEBUG
  DescriptorType type;
#endif
};

struct FrameCommand final {
  ID3D12CommandAllocator *commandAllocator = nullptr;
#if DXR_ENABLED
  ID3D12GraphicsCommandList4 *commandList = nullptr;
#else
  ID3D12GraphicsCommandList2 *commandList = nullptr;
#endif
  bool isListOpen = false;
};

struct FrameResource final {
  FrameCommand fc;
  UINT64 fence = 0;
};

inline HRESULT resetCommandList(FrameCommand *command) {

  assert(!command->isListOpen);
  HRESULT res = command->commandList->Reset(command->commandAllocator, nullptr);
  assert(SUCCEEDED(res));
  command->isListOpen = true;
  return res;
}

// should be used only at the beginning of the frame
inline HRESULT resetAllocatorAndList(FrameCommand *command) {

  assert(!command->isListOpen);

  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished
  // execution on the GPU.
  HRESULT result = command->commandAllocator->Reset();
  assert(SUCCEEDED(result) && "failed resetting allocator");

  // A command list can be reset after it has been added to the command queue
  // via ExecuteCommandList.
  // Reusing the command list reuses memory.
  HRESULT result2 =
      command->commandList->Reset(command->commandAllocator, nullptr);
  assert(SUCCEEDED(result) && "failed resetting allocator");
  command->isListOpen = SUCCEEDED(result) & SUCCEEDED(result2);
  return result2;
}

inline bool executeCommandList(ID3D12CommandQueue *queue,
                               FrameCommand *command) {
  assert(command->isListOpen);
  HRESULT res = command->commandList->Close();
  assert(SUCCEEDED(res) && "Error closing command list");
  ID3D12CommandList *commandLists[] = {command->commandList};
  queue->ExecuteCommandLists(1, commandLists);
  bool succeded = SUCCEEDED(res);
  assert(succeded);
  command->isListOpen = false;
  return succeded;
}

// global declarations
#if DXR_ENABLED
typedef ID3D12Device5 D3D12DeviceType;
#else
typedef ID3D12Device3 D3D12DeviceType;
#endif
extern SIR_ENGINE_API D3D12DeviceType *DEVICE;
extern SIR_ENGINE_API ID3D12Debug *DEBUG_CONTROLLER;
extern IDXGIFactory6 *DXGI_FACTORY;
extern SIR_ENGINE_API IDXGIAdapter3*ADAPTER;
extern UINT64 CURRENT_FENCE;
extern DescriptorHeap *GLOBAL_CBV_SRV_UAV_HEAP;
extern DescriptorHeap *GLOBAL_RTV_HEAP;
extern DescriptorHeap *GLOBAL_DSV_HEAP;
extern ID3D12CommandQueue *GLOBAL_COMMAND_QUEUE;
extern ID3D12Fence *GLOBAL_FENCE;
extern Dx12SwapChain *SWAP_CHAIN;
extern FrameResource FRAME_RESOURCES[FRAME_BUFFERS_COUNT];
extern FrameResource *CURRENT_FRAME_RESOURCE;
// resource managers
extern Dx12TextureManager *TEXTURE_MANAGER;
extern Dx12MeshManager *MESH_MANAGER;
extern Dx12MaterialManager *MATERIAL_MANAGER;
extern DependencyGraph *RENDERING_GRAPH;
extern Dx12ConstantBufferManager *CONSTANT_BUFFER_MANAGER;
extern ShaderManager *SHADER_MANAGER;
extern RootSignatureManager *ROOT_SIGNATURE_MANAGER;
extern Dx12PSOManager *PSO_MANAGER;
extern SIR_ENGINE_API ShadersLayoutRegistry *SHADER_LAYOUT_REGISTRY;
extern BufferManagerDx12 *BUFFER_MANAGER;
extern DebugRenderer *DEBUG_RENDERER;
extern Dx12RenderingContext *RENDERING_CONTEXT;

inline UINT64 insertFenceToGlobalQueue() {
  // Advance the fence value to mark commands up to this fence point.
  CURRENT_FENCE++;

  // Add an instruction to the command queue to set a new fence point. Because
  // we are on the GPU time line, the new fence point won't be set until the
  // GPU finishes processing all the commands prior to this Signal().
  HRESULT res = GLOBAL_COMMAND_QUEUE->Signal(GLOBAL_FENCE, CURRENT_FENCE);
  assert(SUCCEEDED(res));
  return CURRENT_FENCE;
}

inline void flushCommandQueue(ID3D12CommandQueue *queue) {
  // Advance the fence value to mark commands up to this fence point.
  CURRENT_FENCE++;

  // Add an instruction to the command queue to set a new fence point. Because
  // we are on the GPU time line, the new fence point won't be set until the
  // GPU finishes processing all the commands prior to this Signal().
  HRESULT res = queue->Signal(GLOBAL_FENCE, CURRENT_FENCE);
  assert(SUCCEEDED(res));
  auto id = GLOBAL_FENCE->GetCompletedValue();
  // Wait until the GPU has completed commands up to this fence point.
  if (id < CURRENT_FENCE) {
    HANDLE eventHandle =
        CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

    // Fire event when GPU hits current fence.
    res = GLOBAL_FENCE->SetEventOnCompletion(CURRENT_FENCE, eventHandle);
    assert(SUCCEEDED(res));

    // Wait until the GPU hits current fence event is fired.
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

bool initializeGraphicsDx12(BaseWindow *wnd, uint32_t width, uint32_t height);
bool shutdownGraphicsDx12();
bool stopGraphicsDx12();
bool newFrameDx12();
bool dispatchFrameDx12();
void flushDx12();

// headless client functions
bool beginHeadlessWorkDx12();
bool endHeadlessWorkDx12();

RenderingContext *
createDx12RenderingContext(const RenderingContextCreationSettings &settings,
                           uint32_t width, uint32_t height);

class Dx12RenderingContext final : public RenderingContext {
public:
  explicit Dx12RenderingContext(
      const RenderingContextCreationSettings &settings, uint32_t width,
      uint32_t height);
  ~Dx12RenderingContext() = default;
  // private copy and assignment
  Dx12RenderingContext(const Dx12RenderingContext &) = delete;
  Dx12RenderingContext &operator=(const Dx12RenderingContext &) = delete;

  bool initializeGraphics() override;

  void setupCameraForFrame();
  void setupLightingForFrame();
  void bindCameraBuffer(int index) const;
  void bindCameraBufferCompute(int index) const;
  void updateSceneBoundingBox();
  void updateDirectionalLightMatrix();

  inline void setEnviromentMap(const TextureHandle enviromentMapHandle) {
    m_enviromentMapHandle = enviromentMapHandle;
  }

  inline void setEnviromentMapIrradiance(
      const TextureHandle enviromentMapIrradianceHandle) {
    m_enviromentMapIrradianceHandle = enviromentMapIrradianceHandle;
  }

  inline const DirectionalLightData &getLightData() const { return m_light; };
  inline void
  setEnviromentMapRadiance(const TextureHandle enviromentMapRadianceHandle) {
    m_enviromentMapRadianceHandle = enviromentMapRadianceHandle;
  };
  inline TextureHandle getEnviromentMapHandle() const {
    return m_enviromentMapHandle;
  }
  inline TextureHandle getEnviromentMapIrradianceHandle() const {
    return m_enviromentMapIrradianceHandle;
  }
  inline TextureHandle getEnviromentMapRadianceHandle() const {
    return m_enviromentMapRadianceHandle;
  }
  inline void setBrdfHandle(const TextureHandle handle) {
    m_brdfHandle = handle;
  }
  inline TextureHandle getBrdfHandle() const { return m_brdfHandle; }

  inline ConstantBufferHandle getLightCB() const { return m_lightCB; }
  inline BoundingBox getBoundingBox() const { return m_boundingBox; }

  bool newFrame() override;
  bool dispatchFrame() override;
  bool resize(uint32_t width, uint32_t height) override;
  bool stopGraphic() override;
  bool shutdownGraphic() override;
  void flush() override;
  void executeGlobalCommandList() override;
  void resetGlobalCommandList() override;
  void addRenderablesToQueue(const Renderable& renderable) override;
  void renderQueueType(const SHADER_QUEUE_FLAGS flag) override;
  void renderMaterialType(const SHADER_QUEUE_FLAGS queueFlag) override;

private:
  // member variable mostly temporary
  CameraBuffer m_camBufferCPU{};
  ConstantBufferHandle m_cameraHandle{};
  ConstantBufferHandle m_lightBuffer{};
  ConstantBufferHandle m_lightCB{};
  DirectionalLightData m_light;
  TextureHandle m_enviromentMapHandle;
  TextureHandle m_enviromentMapIrradianceHandle;
  TextureHandle m_enviromentMapRadianceHandle;
  TextureHandle m_brdfHandle;
  BoundingBox m_boundingBox;
  DebugDrawHandle m_lightAABBHandle{};

  //TODO possibly find a better way to handle this don't want to leak the std map type out
  //I cannot use my hash map because is quite basic and does not deal with complex datatypes that needs
  //to be constructed, meaning would be hard to put in a ResizableVector as value so for now we don't deal with
  //this
  void* queues;
	
};



	
} // namespace dx12
} // namespace SirEngine
