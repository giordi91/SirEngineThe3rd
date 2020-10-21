#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include <assert.h>

#include "SirEngine/globals.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"

namespace SirEngine {

class IdentityManager;
class BaseWindow;
class AssetManager;
class DependencyGraph;

namespace dx12 {
class Dx12TextureManager;
class Dx12MeshManager;
class Dx12Adapter;
class DescriptorHeap;
class Dx12SwapChain;
class Dx12ConstantBufferManager;
class Dx12ShaderManager;
class Dx12RootSignatureManager;
class ShadersLayoutRegistry;
class Dx12PSOManager;
class BufferManagerDx12;
class Dx12DebugRenderer;
class Dx12RenderingContext;
class Dx12MaterialManager;
class Dx12BindingTableManager;
class Dx12CommandBufferManager;
class Dx12ImGuiManager;

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
#if _DEBUG 
  DescriptorType type;
#endif
};

#if _DEBUG
#define DX_SET_DEBUG_NAME(resource, name) \
  { resource->SetName(frameConvertWide(name)); }
#else
#define DX_SET_DEBUG_NAME(resource, name)
#endif

struct FrameCommand final {
  ID3D12CommandAllocator *commandAllocator = nullptr;
#if DXR_ENABLED
  ID3D12GraphicsCommandList4 *commandList = nullptr;
#else
  ID3D12GraphicsCommandList2 *commandList = nullptr;
#endif
  bool isListOpen = false;
  CommandBufferHandle handle;
};

struct FrameResource final {
  FrameCommand fc;
  UINT64 fence = 0;
};

inline HRESULT resetCommandList(FrameCommand *command);

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
extern  D3D12DeviceType *DEVICE;
extern  ID3D12Debug *DEBUG_CONTROLLER;
extern IDXGIFactory6 *DXGI_FACTORY;
extern  IDXGIAdapter3 *ADAPTER;
extern UINT64 CURRENT_FENCE;
extern DescriptorHeap *GLOBAL_CBV_SRV_UAV_HEAP;
extern DescriptorHeap *GLOBAL_RTV_HEAP;
extern DescriptorHeap *GLOBAL_DSV_HEAP;
extern DescriptorHeap *GLOBAL_SAMPLER_HEAP;
extern ID3D12CommandQueue *GLOBAL_COMMAND_QUEUE;
extern ID3D12Fence *GLOBAL_FENCE;
extern Dx12SwapChain *SWAP_CHAIN;
extern FrameResource FRAME_RESOURCES[FRAME_BUFFERS_COUNT];
extern FrameResource *CURRENT_FRAME_RESOURCE;
extern Dx12CommandBufferManager *COMMAND_BUFFER_MANAGER;
// resource managers
extern Dx12TextureManager *TEXTURE_MANAGER;
extern Dx12MeshManager *MESH_MANAGER;
extern Dx12ConstantBufferManager *CONSTANT_BUFFER_MANAGER;
extern Dx12ShaderManager *SHADER_MANAGER;
extern Dx12RootSignatureManager *ROOT_SIGNATURE_MANAGER;
extern Dx12PSOManager *PSO_MANAGER;
extern BufferManagerDx12 *BUFFER_MANAGER;
extern Dx12DebugRenderer *DEBUG_RENDERER;
extern Dx12BindingTableManager *BINDING_TABLE_MANAGER;
extern Dx12ImGuiManager* IMGUI_MANAGER;

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



RenderingContext *createDx12RenderingContext(
    const RenderingContextCreationSettings &settings, uint32_t width,
    uint32_t height);

class Dx12RenderingContext final : public RenderingContext {
  struct FrameBindingsData {
    FrameBufferBindings m_bindings;
    uint32_t m_magicNumber;
    const char *name;
  };

 public:
  explicit Dx12RenderingContext(
      const RenderingContextCreationSettings &settings, uint32_t width,
      uint32_t height);
  ~Dx12RenderingContext() = default;
  // private copy and assignment
  Dx12RenderingContext(const Dx12RenderingContext &) = delete;
  Dx12RenderingContext &operator=(const Dx12RenderingContext &) = delete;

  bool initializeGraphics() override;

  void setupCameraForFrame() override;

  void updateSceneBoundingBox();
  void updateDirectionalLightMatrix();

  bool newFrame() override;
  bool dispatchFrame() override;
  bool resize(uint32_t width, uint32_t height) override;
  bool stopGraphic() override;
  bool shutdownGraphic() override;
  void flush() override;
  void executeGlobalCommandList() override;
  void resetGlobalCommandList() override;
  void addRenderablesToQueue(const Renderable &renderable) override;
  void renderQueueType(const DrawCallConfig &config,
                       const SHADER_QUEUE_FLAGS flag,
                       const BindingTableHandle passBindings) override;
  void fullScreenPass() override;
  BufferBindingsHandle prepareBindingObject(const FrameBufferBindings &bindings,
                                            const char *name) override;
  void setBindingObject(const BufferBindingsHandle handle) override;
  void clearBindingObject(const BufferBindingsHandle handle) override;
  void freeBindingObject(const BufferBindingsHandle handle) override;
  void setViewportAndScissor(float offsetX, float offsetY, float width,
                             float height, float minDepth,
                             float maxDepth) override;

 private:
  bool initializeGraphicsDx12(BaseWindow *wnd, uint32_t width, uint32_t height);
  inline void assertMagicNumber(const BufferBindingsHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(
               m_bindingsPool.getConstRef(idx).m_magicNumber) == magic &&
           "invalid magic handle for constant buffer");
  }

 public:
  void renderProcedural(const uint32_t indexCount) override;

  void bindCameraBuffer(RSHandle, bool isCompute = false) const override;
  void dispatchCompute(uint32_t blockX, uint32_t blockY,
                       uint32_t blockW) override;

  void renderProceduralIndirect(const BufferHandle &argsBuffer,
                                uint32_t offset = 0) override;

  void bindSamplers(const RSHandle &rs) override;
  static void setHeaps();

 private:
  // member variable mostly temporary
  // CameraBuffer m_camBufferCPU{};
  FrameData m_frameData{};
  ConstantBufferHandle m_cameraHandle{};
  ConstantBufferHandle m_lightBuffer{};
  DebugDrawHandle m_lightAABBHandle{};

  static constexpr uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<FrameBindingsData> m_bindingsPool;
  ID3D12CommandSignature *m_commandIndirect = nullptr;

  // TODO possibly find a better way to handle this don't want to leak the std
  // map type out I cannot use my hash map because is quite basic and does not
  // deal with complex datatypes that needs to be constructed, meaning would be
  // hard to put in a ResizableVector as value so for now we don't deal with
  // this
  void *queues;
  uint32_t m_matrixCounter = 0;
  BufferHandle m_matrixBufferHandle[5]{};
  BindingTableHandle m_frameBindingHandle;
};

}  // namespace dx12
}  // namespace SirEngine
