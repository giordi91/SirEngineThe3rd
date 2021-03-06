#include "platform/windows/graphics/dx12/DX12.h"

#include <glm/gtx/transform.hpp>

#include "SirEngine/Window.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/graphics/lightManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/interopData.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/stringPool.h"
#include "SirEngine/runtimeString.h"
#include "SirEngine/skinClusterManager.h"
#include "dx12BindingTableManager.h"
#include "dx12CommandBufferManager.h"
#include "dx12ImguiManager.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include "platform/windows/graphics/dx12/dx12BufferManager.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12MeshManager.h"
#include "platform/windows/graphics/dx12/dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12ShaderManager.h"
#include "platform/windows/graphics/dx12/dx12SwapChain.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "rootSignatureCompile.h"

#undef max
#undef min

namespace SirEngine::dx12 {

static std::vector<glm::mat4> m(256);

D3D12DeviceType *DEVICE;
ID3D12Debug *DEBUG_CONTROLLER = nullptr;
IDXGIFactory6 *DXGI_FACTORY = nullptr;
IDXGIAdapter3 *ADAPTER = nullptr;
UINT64 CURRENT_FENCE = 0;
DescriptorHeap *GLOBAL_CBV_SRV_UAV_HEAP = nullptr;
DescriptorHeap *GLOBAL_RTV_HEAP = nullptr;
DescriptorHeap *GLOBAL_DSV_HEAP = nullptr;
DescriptorHeap *GLOBAL_SAMPLER_HEAP = nullptr;
ID3D12CommandQueue *GLOBAL_COMMAND_QUEUE = nullptr;
ID3D12Fence *GLOBAL_FENCE = nullptr;
Dx12SwapChain *SWAP_CHAIN = nullptr;
FrameResource FRAME_RESOURCES[FRAME_BUFFERS_COUNT];
FrameResource *CURRENT_FRAME_RESOURCE = nullptr;
Dx12TextureManager *TEXTURE_MANAGER = nullptr;
Dx12MeshManager *MESH_MANAGER = nullptr;
Dx12ConstantBufferManager *CONSTANT_BUFFER_MANAGER = nullptr;
Dx12ShaderManager *SHADER_MANAGER = nullptr;
Dx12PSOManager *PSO_MANAGER = nullptr;
Dx12RootSignatureManager *ROOT_SIGNATURE_MANAGER = nullptr;
BufferManagerDx12 *BUFFER_MANAGER = nullptr;
Dx12DebugRenderer *DEBUG_RENDERER = nullptr;
Dx12BindingTableManager *BINDING_TABLE_MANAGER = nullptr;
Dx12CommandBufferManager *COMMAND_BUFFER_MANAGER = nullptr;
Dx12ImGuiManager *IMGUI_MANAGER = nullptr;


struct Dx12Renderable {
  MeshHandle m_meshHandle;
  MaterialHandle m_materialHandle;
};

typedef std::unordered_map<uint64_t, std::vector<Renderable>>
    Dx12RenderingQueues;

void allocateSamplers() {
  const D3D12_SAMPLER_DESC *samplers = getSamplers();
  for (int i = 0; i < STATIC_SAMPLERS_COUNT; ++i) {
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor;
    GLOBAL_SAMPLER_HEAP->allocateDescriptor(&cpuDescriptor);
    DEVICE->CreateSampler(&samplers[i], cpuDescriptor);
  }
}

bool Dx12RenderingContext::initializeGraphicsDx12(BaseWindow *wnd,
                                                  const uint32_t width,
                                                  const uint32_t height) {
// lets enable debug layer if needed
#if defined(DEBUG) || defined(_DEBUG)
  {
    const HRESULT result =
        D3D12GetDebugInterface(IID_PPV_ARGS(&DEBUG_CONTROLLER));
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

  AdapterRequestConfig adapterConfig{};
  adapterConfig.m_vendor = globals::ENGINE_CONFIG->m_requestedAdapterVendor;
  adapterConfig.m_vendorTolerant = globals::ENGINE_CONFIG->m_vendorTolerant;
  adapterConfig.m_genericRule = globals::ENGINE_CONFIG->m_adapterSelectionRule;

  Dx12AdapterResult adapterResult{};
  bool foundAdapter =
      getBestAdapter(adapterConfig, adapterResult, DXGI_FACTORY);
  assert(foundAdapter && "could not find adapter matching features");

  ADAPTER = adapterResult.m_physicalDevice;
  DEVICE = adapterResult.m_device;

  // log the adapter used
  if (globals::ENGINE_CONFIG->m_verboseStartup) {
    logPhysicalDevice(ADAPTER);
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
    if (opts5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED) assert(0);
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

  COMMAND_BUFFER_MANAGER = new Dx12CommandBufferManager();
  COMMAND_BUFFER_MANAGER->initialize();
  globals::COMMAND_BUFFER_MANAGER = COMMAND_BUFFER_MANAGER;

  // creating global heaps
  GLOBAL_CBV_SRV_UAV_HEAP = new DescriptorHeap();
  GLOBAL_CBV_SRV_UAV_HEAP->initializeAsCBVSRVUAV(1000);

  GLOBAL_RTV_HEAP = new DescriptorHeap();
  GLOBAL_RTV_HEAP->initializeAsRtv(20);

  GLOBAL_DSV_HEAP = new DescriptorHeap();
  GLOBAL_DSV_HEAP->initializeAsDsv(20);

  GLOBAL_SAMPLER_HEAP = new DescriptorHeap();
  GLOBAL_SAMPLER_HEAP->initializeAsSampler(16);

  IMGUI_MANAGER = new Dx12ImGuiManager();
  IMGUI_MANAGER->initialize();
  globals::IMGUI_MANAGER = IMGUI_MANAGER;

  allocateSamplers();

  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
    assert(i < 9);
    char idx[1] = {static_cast<char>(i)};
    auto &fc = FRAME_RESOURCES[i].fc;
    fc.handle = COMMAND_BUFFER_MANAGER->createBuffer(
        CommandBufferManager::COMMAND_BUFFER_ALLOCATION_NONE,
        frameConcatenation("swapChain", idx));
    const auto &data = COMMAND_BUFFER_MANAGER->getData(fc.handle);

    fc.commandAllocator = data.commandAllocator;
    fc.commandList = data.commandList;
    fc.isListOpen = data.isListOpen;
    // createFrameCommand(&FRAME_RESOURCES[i].fc);
  }

  CURRENT_FRAME_RESOURCE = &FRAME_RESOURCES[0];

  // initialize the managers
  dx12::BINDING_TABLE_MANAGER = new Dx12BindingTableManager();
  dx12::BINDING_TABLE_MANAGER->initialize();
  globals::BINDING_TABLE_MANAGER = dx12::BINDING_TABLE_MANAGER;
  // TODO add initialize to all managers for consistency and symmetry
  CONSTANT_BUFFER_MANAGER = new Dx12ConstantBufferManager();
  CONSTANT_BUFFER_MANAGER->initialize();

  BUFFER_MANAGER = new BufferManagerDx12();
  BUFFER_MANAGER->initialize();

  globals::CONSTANT_BUFFER_MANAGER = CONSTANT_BUFFER_MANAGER;
  globals::BUFFER_MANAGER = BUFFER_MANAGER;
  TEXTURE_MANAGER = new Dx12TextureManager();
  TEXTURE_MANAGER->initialize();
  globals::TEXTURE_MANAGER = TEXTURE_MANAGER;
  dx12::MESH_MANAGER = new Dx12MeshManager();
  globals::MESH_MANAGER = dx12::MESH_MANAGER;
  globals::ASSET_MANAGER = new AssetManager();
  globals::ASSET_MANAGER->initialize();

  SHADER_MANAGER = new Dx12ShaderManager();
  SHADER_MANAGER->initialize();
  SHADER_MANAGER->loadShadersInFolder(
      frameConcatenation(globals::ENGINE_CONFIG->m_dataSourcePath,
                         "/processed/shaders/DX12/rasterization"));
  SHADER_MANAGER->loadShadersInFolder(
      frameConcatenation(globals::ENGINE_CONFIG->m_dataSourcePath,
                         "/processed/shaders/DX12/compute"));
  globals::SHADER_MANAGER = dx12::SHADER_MANAGER;

  globals::LIGHT_MANAGER = new graphics::LightManager();

  ROOT_SIGNATURE_MANAGER = new Dx12RootSignatureManager();
  ROOT_SIGNATURE_MANAGER->initialize();
  globals::ROOT_SIGNATURE_MANAGER = ROOT_SIGNATURE_MANAGER;

  PSO_MANAGER = new Dx12PSOManager();
  PSO_MANAGER->initialize();
  globals::PSO_MANAGER = PSO_MANAGER;

  if (globals::ENGINE_CONFIG->m_useCachedPSO) {
    assert(0 &&
           "to used cached pso we need to add root signature information in "
           "there");
    PSO_MANAGER->loadCachedPSOInFolder(frameConcatenation(
        globals::ENGINE_CONFIG->m_dataSourcePath, "/processed/pso/DX12"));
  } else {
    PSO_MANAGER->loadRawPSOInFolder(
        frameConcatenation(globals::ENGINE_CONFIG->m_dataSourcePath, "/pso"));
  }

  // mesh manager needs to load after pso and RS since it initialize material
  // types
  globals::MATERIAL_MANAGER = new MaterialManager();
  globals::MATERIAL_MANAGER->inititialize();

  globals::DEBUG_RENDERER = new DebugRenderer();
  globals::DEBUG_RENDERER->initialize();

  globals::ANIMATION_MANAGER = new AnimationManager();
  globals::ANIMATION_MANAGER->init();

  globals::INTEROP_DATA = new InteropData();
  globals::INTEROP_DATA->initialize();

  globals::SKIN_MANAGER = new SkinClusterManager();
  globals::SKIN_MANAGER->init();

  const bool isHeadless = (wnd == nullptr) | (width == 0) | (height == 0);

  if (!isHeadless) {
    // init swap chain
    const NativeWindow *nativeWindow = wnd->getNativeWindow();
    assert(sizeof(HWND) == 8);
    HWND handle;
    memcpy(&handle, &nativeWindow->data2, sizeof(HWND));

    dx12::SWAP_CHAIN = new dx12::Dx12SwapChain();
    dx12::SWAP_CHAIN->initialize(handle, width, height);

    COMMAND_BUFFER_MANAGER->resetBufferHandle(
        dx12::CURRENT_FRAME_RESOURCE->fc.handle);
    dx12::SWAP_CHAIN->resize(&dx12::CURRENT_FRAME_RESOURCE->fc, width, height);
  } else {
    SE_CORE_INFO("Requested HEADLESS client, no swapchain is initialized");
  }

  // create the draw indirect commands
  D3D12_INDIRECT_ARGUMENT_DESC Args[1];
  Args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

  D3D12_COMMAND_SIGNATURE_DESC ProgramDesc;
  ProgramDesc.ByteStride = 36;
  ProgramDesc.NumArgumentDescs = 1;
  ProgramDesc.pArgumentDescs = Args;
  ProgramDesc.NodeMask = 0;  // if single gpu set to 0;
  DEVICE->CreateCommandSignature(&ProgramDesc, nullptr,
                                 IID_PPV_ARGS(&m_commandIndirect));

  // allocate memory for per frame matrix uploads
  uint32_t matrixCount = globals::ENGINE_CONFIG->m_matrixBufferSize;
  uint32_t matrixSize = sizeof(glm::mat4) * matrixCount;

  assert(globals::ENGINE_CONFIG->m_frameBufferingCount <= 5);
  for (uint32_t i = 0; i < globals::ENGINE_CONFIG->m_frameBufferingCount; ++i) {
    m_matrixBufferHandle[i] = globals::BUFFER_MANAGER->allocate(
        matrixSize, nullptr, "perFrameMatrixBuffer", matrixCount,
        sizeof(glm::mat4),
        BufferManager::BUFFER_FLAGS_BITS::STORAGE_BUFFER |
            BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY);
  }
  return true;
}

bool beginHeadlessWorkDx12() {
  // here we need to check which frame resource we are going to use
  dx12::CURRENT_FRAME_RESOURCE = &FRAME_RESOURCES[globals::CURRENT_FRAME];

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
  COMMAND_BUFFER_MANAGER->resetBufferHandle(CURRENT_FRAME_RESOURCE->fc.handle);
  // Indicate a state transition on the resource usage.
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;

  auto *heap = dx12::GLOBAL_CBV_SRV_UAV_HEAP->getResource();
  commandList->SetDescriptorHeaps(1, &heap);

  return true;
}

bool endHeadlessWorkDx12() {
  assert(0);
  /*
// Done recording commands.
dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE,
                         &dx12::CURRENT_FRAME_RESOURCE->fc);

dx12::CURRENT_FRAME_RESOURCE->fence = ++dx12::CURRENT_FENCE;
dx12::GLOBAL_COMMAND_QUEUE->Signal(dx12::GLOBAL_FENCE, dx12::CURRENT_FENCE);
// bump the frame
globals::CURRENT_FRAME = (globals::CURRENT_FRAME + 1) % FRAME_BUFFERS_COUNT;
*/
  return true;
}

void flushCommandQueue(ID3D12CommandQueue *queue) {
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

RenderingContext *createDx12RenderingContext(
    const RenderingContextCreationSettings &settings, uint32_t width,
    uint32_t height) {
  auto *ctx = new Dx12RenderingContext(settings, width, height);
  return ctx;
}

Dx12RenderingContext::Dx12RenderingContext(
    const RenderingContextCreationSettings &settings, const uint32_t width,
    const uint32_t height)
    : RenderingContext(settings, width, height), m_bindingsPool(RESERVE_SIZE) {
  SE_CORE_INFO("Initializing a DirectX 12 context");
  queues = new Dx12RenderingQueues();
}

bool Dx12RenderingContext::initializeGraphics() {
  const bool result = initializeGraphicsDx12(
      m_settings.window, m_screenInfo.width, m_screenInfo.height);
  if (!result) {
    SE_CORE_ERROR("FATAL: could not initialize graphics");
  }

  graphics::BindingDescription perFrameDescriptrion[] = {
      {3, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,  // grass config
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX |
           GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT |
           GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
      {1, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,  // albedo texture
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX |
           GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT |
           GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
  };
  m_frameBindingHandle = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      perFrameDescriptrion, ARRAYSIZE(perFrameDescriptrion),
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "perFrameDataBindingTable");

  // initialize camera and light
  // ask for the camera buffer handle;
  m_cameraHandle =
      globals::CONSTANT_BUFFER_MANAGER->allocate(sizeof(m_frameData));

  float intensity = 4.0f;
  m_light.lightColor = {intensity, intensity, intensity, 1.0f};
  // m_light.lightDir = {0.0f, 0.0f, -1.0f, 0.0f};
  m_light.lightDir = {-1.0f, -0.6f, -1.0f, 1.0f};
  m_light.lightPosition = {10.0f, 10.0f, 10.0f, 1.0f};

  // build a look at matrix for the light
  glm::vec3 lightDir = glm::normalize(glm::vec3(m_light.lightDir));
  glm::vec3 upVector{0, 1, 0};

  const auto cross = glm::cross(upVector, lightDir);
  const auto crossNorm = glm::normalize(cross);

  const auto newUp = glm::cross(lightDir, crossNorm);
  const auto newUpNorm = glm::normalize(newUp);

  m_light.localToWorld =
      glm::mat4(glm::vec4(crossNorm, 0), glm::vec4(newUpNorm, 0),
                glm::vec4(lightDir, 0), m_light.lightPosition);

  m_light.worldToLocal = glm::inverse(m_light.localToWorld);

  // allocate the constant buffer
  m_lightCB = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(DirectionalLightData), 0, &m_light);

  return result;
}

void Dx12RenderingContext::setupCameraForFrame(const uint32_t renderWidth, const uint32_t renderHeight, const bool updateCameraMovment) {
  globals::ACTIVE_CAMERA->updateCamera(renderWidth,renderHeight,updateCameraMovment);

  m_frameData.m_mainCamera = globals::MAIN_CAMERA->getCameraBuffer();
  m_frameData.m_activeCamera = globals::ACTIVE_CAMERA->getCameraBuffer();

  m_frameData.screenWidth =
      static_cast<float>(renderWidth);
  m_frameData.screenHeight =
      static_cast<float>(renderHeight);
  m_frameData.time =
      static_cast<float>(globals::GAME_CLOCK.getDeltaFromOrigin() * 1e-9);

  globals::CONSTANT_BUFFER_MANAGER->update(m_cameraHandle, &m_frameData);
}

void Dx12RenderingContext::updateSceneBoundingBox() {
  uint32_t boxesCount = 0;
  const BoundingBox *boxes = dx12::MESH_MANAGER->getBoundingBoxes(boxesCount);
  float minX = std::numeric_limits<float>::max();
  float minY = minX;
  float minZ = minY;

  float maxX = std::numeric_limits<float>::min();
  float maxY = maxX;
  float maxZ = maxY;

  for (uint32_t i = 0; i < boxesCount; ++i) {
    const BoundingBox &box = boxes[i];
    // lets us compute bounding box
    minX = box.min.x < minX ? box.min.x : minX;
    minY = box.min.y < minY ? box.min.y : minY;
    minZ = box.min.z < minZ ? box.min.z : minZ;

    maxX = box.min.x > maxX ? box.min.x : maxX;
    maxY = box.min.y > maxY ? box.min.y : maxY;
    maxZ = box.min.z > maxZ ? box.min.z : maxZ;

    minX = box.max.x < minX ? box.max.x : minX;
    minY = box.max.y < minY ? box.max.y : minY;
    minZ = box.max.z < minZ ? box.max.z : minZ;

    maxX = box.max.x > maxX ? box.max.x : maxX;
    maxY = box.max.y > maxY ? box.max.y : maxY;
    maxZ = box.max.z > maxZ ? box.max.z : maxZ;
  }
  m_boundingBox.min.x = minX;
  m_boundingBox.min.y = minY;
  m_boundingBox.min.z = minZ;
  m_boundingBox.max.x = maxX;
  m_boundingBox.max.y = maxY;
  m_boundingBox.max.z = maxZ;
}

void expandBoundingBox(const BoundingBox &aabb, glm::vec3 *points) {
  points[0] = aabb.min;
  points[1] = aabb.max;
  points[2] = glm::vec3(points[0].x, points[0].y, points[1].z);
  points[3] = glm::vec3(points[0].x, points[1].y, points[0].z);
  points[4] = glm::vec3(points[1].x, points[0].y, points[0].z);
  points[5] = glm::vec3(points[0].x, points[1].y, points[1].z);
  points[6] = glm::vec3(points[1].x, points[0].y, points[1].z);
  points[7] = glm::vec3(points[1].x, points[1].y, points[0].z);
}

void Dx12RenderingContext::updateDirectionalLightMatrix() {
  // we need to compute the scene bounding box in light space
  glm::vec3 expanded[8];
  expandBoundingBox(m_boundingBox, expanded);
  float minX = std::numeric_limits<float>::max();
  float minY = minX;
  float minZ = minY;

  float maxX = std::numeric_limits<float>::min();
  float maxY = maxX;
  float maxZ = maxY;

  for (int i = 0; i < 8; ++i) {
    const glm::vec3 &point = expanded[i];

    const glm::vec4 localPointV = m_light.worldToLocal * glm::vec4(point, 1.0f);

    // lets us compute bounding box
    minX = localPointV.x < minX ? localPointV.x : minX;
    minY = localPointV.y < minY ? localPointV.y : minY;
    minZ = localPointV.z < minZ ? localPointV.z : minZ;

    maxX = localPointV.x > maxX ? localPointV.x : maxX;
    maxY = localPointV.y > maxY ? localPointV.y : maxY;
    maxZ = localPointV.z > maxZ ? localPointV.z : maxZ;
  }
  BoundingBox m_lightAABB;
  m_lightAABB.min.x = minX;
  m_lightAABB.min.y = minY;
  m_lightAABB.min.z = minZ;
  m_lightAABB.max.x = maxX;
  m_lightAABB.max.y = maxY;
  m_lightAABB.max.z = maxZ;

  // debug rendering of the light
  expandBoundingBox(m_lightAABB, expanded);

  for (int i = 0; i < 8; ++i) {
    glm::vec3 &point = expanded[i];
    const glm::vec4 point4 = glm::vec4(point.x, point.y, point.z, 1.0f);

    auto localPointV = m_light.localToWorld * point4;

    point.x = localPointV.x;
    point.y = localPointV.y;
    point.z = localPointV.z;
  }

  // we have the bounding box in light space we want to render it
  // m_lightAABBHandle =
  //    globals::DEBUG_RENDERER->drawAnimatedBoundingBoxFromFullPoints(
  //        m_lightAABBHandle, expanded, 1, glm::vec4(1, 0, 0, 1), "");

  // we can now use min max to generate the projection matrix needed;
  auto ortho =
      getOrthoMatrix(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ));
  m_light.projectionMatrix = glm::transpose(ortho);
  m_light.lightVP = glm::transpose(ortho * m_light.worldToLocal);
}

void Dx12RenderingContext::addRenderablesToQueue(const Renderable &renderable) {
  auto *typedQueues = reinterpret_cast<Dx12RenderingQueues *>(queues);

  // store the renderable on each queue
  for (int i = 0; i < MaterialManager::QUEUE_COUNT; ++i) {
    PSOHandle pso = globals::MATERIAL_MANAGER->getmaterialPSO(
        renderable.m_materialHandle, static_cast<SHADER_QUEUE_FLAGS>(1 << i));
    if (pso.isHandleValid()) {
      // compute flag, is going to be a combination: of the index and the
      // psohandle
      uint64_t psoHash = static_cast<uint64_t>(pso.handle) << 32;
      uint64_t queue = (1ull << i);
      uint64_t key = queue | psoHash;
      (*typedQueues)[key].emplace_back(renderable);
    }
  }
}

void Dx12RenderingContext::renderQueueType(
    const DrawCallConfig &config, const SHADER_QUEUE_FLAGS flag,
    const BindingTableHandle passBindings) {
  const auto &typedQueues = *(static_cast<Dx12RenderingQueues *>(queues));

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  ID3D12GraphicsCommandList2 *commandList = currentFc->commandList;

  setViewportAndScissor(0, 0, static_cast<float>(config.width),
                        static_cast<float>(config.height), 0, 1);
  setHeaps();

  UINT counter = 0;
  for (const auto &renderableList : typedQueues) {
    if (globals::MATERIAL_MANAGER->isQueueType(renderableList.first, flag)) {
      // now that we know the material goes in the the deferred queue we can
      // start rendering it

      // bind the corresponding RS and PSO
      ShaderBind bind = globals::MATERIAL_MANAGER->bindRSandPSO(
          renderableList.first, renderableList.second[0].m_materialHandle);
      annotateGraphicsBegin(globals::PSO_MANAGER->getPSOName(bind.pso));

      // binding the camera
      bindCameraBuffer(bind.rs, false);

      if (passBindings.isHandleValid()) {
        // binding the per pass data
        globals::BINDING_TABLE_MANAGER->bindTable(
            PSOManager::PER_PASS_BINDING_INDEX, passBindings, bind.rs);
      }
      bindSamplers(bind.rs);

      // looping each of the object
      const size_t count = renderableList.second.size();
      const Renderable *currRenderables = renderableList.second.data();
      for (size_t i = 0; i < count; ++i) {
        const Renderable &renderable = currRenderables[i];

        // bind material data like textures etc, then render
        globals::MATERIAL_MANAGER->bindMaterial(renderable.m_materialHandle,
                                                flag);

        commandList->SetGraphicsRoot32BitConstant(
            PSOManager::PER_OBJECT_BINDING_INDEX + 1, counter, 0);

        MESH_MANAGER->render(renderable.m_meshHandle, currentFc);
        counter += 1;
      }
      annotateGraphicsEnd();
    }
  }
}

void Dx12RenderingContext::fullScreenPass() {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->DrawInstanced(6, 1, 0, 0);
}

BufferBindingsHandle Dx12RenderingContext::prepareBindingObject(
    const FrameBufferBindings &bindings, const char *name) {
  uint32_t index;
  FrameBindingsData &data = m_bindingsPool.getFreeMemoryData(index);
  data.name = persistentString(name);
  data.m_bindings = bindings;
  data.m_magicNumber = MAGIC_NUMBER_COUNTER++;
  return {data.m_magicNumber << 16 | index};
}

void Dx12RenderingContext::setBindingObject(const BufferBindingsHandle handle) {
  assertMagicNumber(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  const FrameBindingsData &data = m_bindingsPool.getConstRef(idx);

  annotateGraphicsBegin(data.name);

  D3D12_CPU_DESCRIPTOR_HANDLE handles[10] = {};
  D3D12_RESOURCE_BARRIER barriers[20]{};
  assert(10 + data.m_bindings.extraBindingsCount < 20);

  int counter = 0;
  int barrierCounter = 0;
  // processing render targets
  for (int i = 0; i < 8; ++i) {
    const RTBinding &binding = data.m_bindings.colorRT[i];

    if ((!binding.handle.isHandleValid()) & !binding.isSwapChainBackBuffer) {
      continue;
    }
    const TextureHandle destination =
        binding.isSwapChainBackBuffer
            ? dx12::SWAP_CHAIN->currentBackBufferTexture()
            : binding.handle;

    handles[counter] = dx12::TEXTURE_MANAGER->getRTVDx12(destination).cpuHandle;

    // TODO we are still doing state tracking in dx12 we should not do that
    // anymore and let get requested state as must requirement
    // barrierCounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
    //    destination, toDx12ResourceState(binding.neededResourceState),
    //    barriers, barrierCounter);

    if (binding.currentResourceState != binding.neededResourceState) {
      dx12::TEXTURE_MANAGER->transitionTexture({}, destination,
                                               binding.currentResourceState,
                                               binding.neededResourceState);
    }
    counter++;
  }

  // processing extra possible bindings, this are bindings that don't go as
  // output but possibly as inputs like dynamic resources that need to be
  // transitioned from write to read
  for (uint32_t i = 0; i < data.m_bindings.extraBindingsCount; ++i) {
    const RTBinding &binding = data.m_bindings.extraBindings[i];
    /*
    barrierCounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
        binding.handle, toDx12ResourceState(binding.neededResourceState),
        barriers, barrierCounter);
    */


    if (binding.currentResourceState != binding.neededResourceState) {
      dx12::TEXTURE_MANAGER->transitionTexture({}, binding.handle,
                                               binding.currentResourceState,
                                               binding.neededResourceState);
    }
  }

  // processing the depth
  D3D12_CPU_DESCRIPTOR_HANDLE depthHandle = {0};
  if (data.m_bindings.depthStencil.handle.isHandleValid()) {
    const DepthBinding &binding = data.m_bindings.depthStencil;
    depthHandle = dx12::TEXTURE_MANAGER->getRTVDx12(binding.handle).cpuHandle;
    /*
    barrierCounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
        binding.handle, toDx12ResourceState(binding.neededResourceState),
        barriers, barrierCounter);
        */

    if (binding.currentResourceState != binding.neededResourceState) {
      dx12::TEXTURE_MANAGER->transitionTexture({}, binding.handle,
                                               binding.currentResourceState,
                                               binding.neededResourceState);
    }
  }

  // transitioning resources if needed
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  if (barrierCounter != 0) {
    commandList->ResourceBarrier(barrierCounter, barriers);
  }
  // clear if needed, this needs to happen after the barrier transitions
  // otherwise there will be an invalid state and debug layer complaints
  for (int i = 0; i < 8; ++i) {
    const RTBinding &binding = data.m_bindings.colorRT[i];
    if (!binding.handle.isHandleValid() & !binding.isSwapChainBackBuffer) {
      continue;
    }
    if (binding.shouldClearColor) {
      const TextureHandle destination =
          binding.isSwapChainBackBuffer
              ? dx12::SWAP_CHAIN->currentBackBufferTexture()
              : binding.handle;

      dx12::TEXTURE_MANAGER->clearRT(destination, &binding.clearColor.x);
    }
  }

  if (data.m_bindings.depthStencil.handle.isHandleValid()) {
    const DepthBinding &binding = data.m_bindings.depthStencil;
    if (binding.shouldClearDepth) {
      dx12::TEXTURE_MANAGER->clearDepth(binding.handle,
                                        binding.clearDepthColor.x,
                                        binding.clearStencilColor.x);
    }
  }

  if (counter > 0) {
    D3D12_CPU_DESCRIPTOR_HANDLE *depthAddress =
        depthHandle.ptr != 0 ? &depthHandle : nullptr;
    commandList->OMSetRenderTargets(counter, handles, false, depthAddress);
  }
}

void Dx12RenderingContext::clearBindingObject(const BufferBindingsHandle) {
  annotateGraphicsEnd();
}

void Dx12RenderingContext::freeBindingObject(
    const BufferBindingsHandle handle) {
  assertMagicNumber(handle);
  const uint32_t magic = getMagicFromHandle(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  const FrameBindingsData &data = m_bindingsPool.getConstRef(idx);
  stringFree(data.name);

  m_bindingsPool.free(idx);
}
auto Dx12RenderingContext::setViewportAndScissor(
    const float offsetX, const float offsetY, const float width,
    const float height, const float minDepth, const float maxDepth) -> void {
  D3D12_VIEWPORT viewport;
  viewport.MaxDepth = maxDepth;
  viewport.MinDepth = minDepth;
  viewport.Height = height;
  viewport.Width = width;
  viewport.TopLeftX = offsetX;
  viewport.TopLeftY = offsetY;
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  currentFc->commandList->RSSetViewports(1, &viewport);
  auto scissor =
      D3D12_RECT{static_cast<uint16_t>(offsetX), static_cast<uint16_t>(offsetY),
                 static_cast<uint16_t>(offsetX + width),
                 static_cast<uint16_t>(offsetY + height)};
  currentFc->commandList->RSSetScissorRects(1, &scissor);
}

void Dx12RenderingContext::renderProcedural(const uint32_t indexCount) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  currentFc->commandList->DrawInstanced(indexCount, 1, 0, 0);
}

void Dx12RenderingContext::bindCameraBuffer(RSHandle rs,
                                            const bool isCompute) const {
  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(m_frameBindingHandle,
                                                     m_cameraHandle, 0, 0);
  globals::BINDING_TABLE_MANAGER->bindBuffer(
      m_frameBindingHandle, m_matrixBufferHandle[globals::CURRENT_FRAME], 1, 1);
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_FRAME_DATA_BINDING_INDEX, m_frameBindingHandle, rs,
      isCompute);
}

void Dx12RenderingContext::dispatchCompute(const uint32_t blockX,
                                           const uint32_t blockY,
                                           const uint32_t blockZ) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto *commandList = currentFc->commandList;
  commandList->Dispatch(blockX, blockY, blockZ);
}

void Dx12RenderingContext::renderProceduralIndirect(
    const BufferHandle &argsBuffer, const uint32_t offset) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;

  ID3D12Resource *buff = dx12::BUFFER_MANAGER->getNativeBuffer(argsBuffer);
  currentFc->commandList->ExecuteIndirect(m_commandIndirect, 1, buff, offset,
                                          nullptr, 0);
}

void Dx12RenderingContext::bindSamplers(const RSHandle &rs) {
  // binding samplers
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  int samplersBindSlot = dx12::ROOT_SIGNATURE_MANAGER->getBindingSlot(rs, 1);
  if (samplersBindSlot != -1) {
    auto samplersHandle = dx12::GLOBAL_SAMPLER_HEAP->getGpuStart();
    commandList->SetGraphicsRootDescriptorTable(samplersBindSlot,
                                                samplersHandle);
  }
}

bool Dx12RenderingContext::newFrame() {
  globals::STRING_POOL->resetFrameMemory();
  globals::FRAME_ALLOCATOR->reset();
  // here we need to check which frame resource we are going to use
  dx12::CURRENT_FRAME_RESOURCE = &dx12::FRAME_RESOURCES[globals::CURRENT_FRAME];

  // check if the resource has finished rendering if not we have to wait
  if (dx12::CURRENT_FRAME_RESOURCE->fence != 0 &&
      dx12::GLOBAL_FENCE->GetCompletedValue() <
          dx12::CURRENT_FRAME_RESOURCE->fence) {
    const HANDLE eventHandle =
        CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    const auto handleResult = dx12::GLOBAL_FENCE->SetEventOnCompletion(
        dx12::CURRENT_FRAME_RESOURCE->fence, eventHandle);
    assert(SUCCEEDED(handleResult));
    WaitForSingleObject(eventHandle, INFINITE);

    CloseHandle(eventHandle);
  }
  // at this point we know we are ready to go

  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished
  // execution on the GPU.
  COMMAND_BUFFER_MANAGER->resetBufferHandle(
      dx12::CURRENT_FRAME_RESOURCE->fc.handle);
  // Indicate a state transition on the resource usage.
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
  D3D12_RESOURCE_BARRIER rtbarrier[1];

  const TextureHandle backBufferH =
      dx12::SWAP_CHAIN->currentBackBufferTexture();
  int rtcounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      backBufferH, D3D12_RESOURCE_STATE_RENDER_TARGET, rtbarrier, 0);
  if (rtcounter != 0) {
    commandList->ResourceBarrier(rtcounter, rtbarrier);
  }

  // Set the viewport and scissor rect.  This needs to be reset whenever the
  // command list is reset.
  commandList->RSSetViewports(1, dx12::SWAP_CHAIN->getViewport());
  commandList->RSSetScissorRects(1, dx12::SWAP_CHAIN->getScissorRect());

  ID3D12DescriptorHeap *heaps[2] = {GLOBAL_CBV_SRV_UAV_HEAP->getResource(),
                                    GLOBAL_SAMPLER_HEAP->getResource()};
  commandList->SetDescriptorHeaps(2, heaps);

  static float angle = 0.0f;
  angle += 0.01;
  for (int i = 0; i < 32; ++i) {
    m[i] = glm::transpose(glm::translate(glm::mat4(1), glm::vec3(i * 6, 0, 0)));
  }

  auto h = m_matrixBufferHandle[globals::CURRENT_FRAME];
  dx12::BUFFER_MANAGER->update(h, m.data(), 0, sizeof(float) * 16 * 32);

  // resetting the matrixbuffer counter
  m_matrixCounter = 0;

  return true;
}
void Dx12RenderingContext::setHeaps() {
  ID3D12DescriptorHeap *heaps[2] = {GLOBAL_CBV_SRV_UAV_HEAP->getResource(),
                                    GLOBAL_SAMPLER_HEAP->getResource()};
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
  commandList->SetDescriptorHeaps(2, heaps);
}

bool Dx12RenderingContext::dispatchFrame() {
  D3D12_RESOURCE_BARRIER rtbarrier[1];
  // finally transition the resource to be present
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;

  TextureHandle backBufferH = dx12::SWAP_CHAIN->currentBackBufferTexture();
  const int rtcounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      backBufferH, D3D12_RESOURCE_STATE_PRESENT, rtbarrier, 0);
  if (rtcounter != 0) {
    commandList->ResourceBarrier(rtcounter, rtbarrier);
  }

  // Done recording commands.
  COMMAND_BUFFER_MANAGER->executeBuffer(
      dx12::CURRENT_FRAME_RESOURCE->fc.handle);

  dx12::CURRENT_FRAME_RESOURCE->fence = ++dx12::CURRENT_FENCE;
  dx12::GLOBAL_COMMAND_QUEUE->Signal(dx12::GLOBAL_FENCE, dx12::CURRENT_FENCE);
  // swap the back and front buffers
  dx12::SWAP_CHAIN->present();
  // bump the frame
  globals::TOTAL_NUMBER_OF_FRAMES += 1;
  globals::CURRENT_FRAME = (globals::CURRENT_FRAME + 1) % FRAME_BUFFERS_COUNT;

  // TODO make sure the behaviour is symmetrical with VK
  dx12::BUFFER_MANAGER->clearUploadRequests();
  // dx12::CONSTANT_BUFFER_MANAGER->clearUpQueueFree();
  return true;
}

bool Dx12RenderingContext::resize(const uint32_t width, const uint32_t height) {
  globals::RENDERING_CONTEXT->flush();
  return dx12::SWAP_CHAIN->resize(&dx12::CURRENT_FRAME_RESOURCE->fc, width,
                                  height);
}

bool Dx12RenderingContext::stopGraphic() {
  globals::RENDERING_CONTEXT->flush();
  return true;
}

bool Dx12RenderingContext::shutdownGraphic() {
  globals::RENDERING_CONTEXT->flush();

  delete SWAP_CHAIN;

  MESH_MANAGER->cleanup();
  delete MESH_MANAGER;

  TEXTURE_MANAGER->cleanup();
  delete TEXTURE_MANAGER;

  CONSTANT_BUFFER_MANAGER->cleanup();
  delete CONSTANT_BUFFER_MANAGER;

  SHADER_MANAGER->cleanup();
  delete SHADER_MANAGER;

  PSO_MANAGER->cleanup();
  delete PSO_MANAGER;

  ROOT_SIGNATURE_MANAGER->cleanup();
  delete ROOT_SIGNATURE_MANAGER;

  BUFFER_MANAGER->cleanup();
  delete BUFFER_MANAGER;

  BINDING_TABLE_MANAGER->cleanup();
  delete BINDING_TABLE_MANAGER;

  COMMAND_BUFFER_MANAGER->cleanup();
  delete COMMAND_BUFFER_MANAGER;

  return true;
}

void Dx12RenderingContext::flush() {
  flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
}

void Dx12RenderingContext::executeGlobalCommandList() {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  // executeCommandList(dx12::GLOBAL_COMMAND_QUEUE, currentFc);
  COMMAND_BUFFER_MANAGER->executeBuffer(currentFc->handle);
}

void Dx12RenderingContext::resetGlobalCommandList() {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  COMMAND_BUFFER_MANAGER->resetBufferHandle(currentFc->handle);
  setHeaps();
}
}  // namespace SirEngine::dx12
