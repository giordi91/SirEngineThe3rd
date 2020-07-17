#include "platform/windows/graphics/dx12/DX12.h"

#include "SirEngine/Window.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/lightManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/stringPool.h"
#include "SirEngine/runtimeString.h"
#include "SirEngine/skinClusterManager.h"
#include "dx12BindingTableManager.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include "platform/windows/graphics/dx12/dx12BufferManager.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12DebugRenderer.h"
#include "platform/windows/graphics/dx12/dx12MaterialManager.h"
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

D3D12DeviceType *DEVICE;
ID3D12Debug *DEBUG_CONTROLLER = nullptr;
IDXGIFactory6 *DXGI_FACTORY = nullptr;
IDXGIAdapter3 *ADAPTER = nullptr;
UINT64 CURRENT_FENCE = 0;
DescriptorHeap *GLOBAL_CBV_SRV_UAV_HEAP = nullptr;
DescriptorHeap *GLOBAL_RTV_HEAP = nullptr;
DescriptorHeap *GLOBAL_DSV_HEAP = nullptr;
ID3D12CommandQueue *GLOBAL_COMMAND_QUEUE = nullptr;
ID3D12Fence *GLOBAL_FENCE = nullptr;
Dx12SwapChain *SWAP_CHAIN = nullptr;
FrameResource FRAME_RESOURCES[FRAME_BUFFERS_COUNT];
FrameResource *CURRENT_FRAME_RESOURCE = nullptr;
Dx12TextureManager *TEXTURE_MANAGER = nullptr;
Dx12MeshManager *MESH_MANAGER = nullptr;
Dx12MaterialManager *MATERIAL_MANAGER = nullptr;
Dx12ConstantBufferManager *CONSTANT_BUFFER_MANAGER = nullptr;
Dx12ShaderManager *SHADER_MANAGER = nullptr;
Dx12PSOManager *PSO_MANAGER = nullptr;
Dx12RootSignatureManager *ROOT_SIGNATURE_MANAGER = nullptr;
BufferManagerDx12 *BUFFER_MANAGER = nullptr;
Dx12DebugRenderer *DEBUG_RENDERER = nullptr;
Dx12RenderingContext *RENDERING_CONTEXT = nullptr;
Dx12BindingTableManager *BINDING_TABLE_MANAGER = nullptr;

struct Dx12Renderable {
  Dx12MeshRuntime m_meshRuntime;
  Dx12MaterialRuntime m_materialRuntime;
};

typedef std::unordered_map<uint32_t, std::vector<Dx12Renderable>>
    Dx12RenderingQueues;

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

bool initializeGraphicsDx12(BaseWindow *wnd, const uint32_t width,
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
  ROOT_SIGNATURE_MANAGER->loadSignaturesInFolder(frameConcatenation(
      globals::ENGINE_CONFIG->m_dataSourcePath, "/processed/rs"));
  globals::ROOT_SIGNATURE_MANAGER = ROOT_SIGNATURE_MANAGER;

  PSO_MANAGER = new Dx12PSOManager();
  PSO_MANAGER->initialize();
  globals::PSO_MANAGER = PSO_MANAGER;

  if (globals::ENGINE_CONFIG->m_useCachedPSO) {
    assert(0 && "to used cached pso we need to add root signature information in there");
    PSO_MANAGER->loadCachedPSOInFolder(frameConcatenation(
        globals::ENGINE_CONFIG->m_dataSourcePath, "/processed/pso/DX12"));
  } else {
    PSO_MANAGER->loadRawPSOInFolder(
        frameConcatenation(globals::ENGINE_CONFIG->m_dataSourcePath, "/pso"));
  }

  // mesh manager needs to load after pso and RS since it initialize material
  // types
  MATERIAL_MANAGER = new Dx12MaterialManager();
  MATERIAL_MANAGER->inititialize();
  MATERIAL_MANAGER->loadTypesInFolder(frameConcatenation(
      globals::ENGINE_CONFIG->m_dataSourcePath, "/materials/types"));
  globals::MATERIAL_MANAGER = MATERIAL_MANAGER;

  DEBUG_RENDERER = new Dx12DebugRenderer();
  DEBUG_RENDERER->initialize();
  globals::DEBUG_RENDERER = DEBUG_RENDERER;

  globals::ANIMATION_MANAGER = new AnimationManager();
  globals::ANIMATION_MANAGER->init();

  globals::SKIN_MANAGER = new SkinClusterManager();
  globals::SKIN_MANAGER->init();

  globals::DEBUG_FRAME_DATA = new globals::DebugFrameData();

  const bool isHeadless = (wnd == nullptr) | (width == 0) | (height == 0);

  if (!isHeadless) {
    // init swap chain
    const NativeWindow *nativeWindow = wnd->getNativeWindow();
    assert(sizeof(HWND) == 8);
    HWND handle;
    memcpy(&handle, &nativeWindow->data2, sizeof(HWND));

    dx12::SWAP_CHAIN = new dx12::Dx12SwapChain();
    dx12::SWAP_CHAIN->initialize(handle, width, height);
    dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
    dx12::SWAP_CHAIN->resize(&dx12::CURRENT_FRAME_RESOURCE->fc, width, height);
  } else {
    SE_CORE_INFO("Requested HEADLESS client, no swapchain is initialized");
  }

  return true;
}
void flushDx12() { flushCommandQueue(GLOBAL_COMMAND_QUEUE); }

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
bool newFrameDx12() { return true; }
bool dispatchFrameDx12() {
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
  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE,
                           &dx12::CURRENT_FRAME_RESOURCE->fc);

  dx12::CURRENT_FRAME_RESOURCE->fence = ++dx12::CURRENT_FENCE;
  dx12::GLOBAL_COMMAND_QUEUE->Signal(dx12::GLOBAL_FENCE, dx12::CURRENT_FENCE);
  // swap the back and front buffers
  dx12::SWAP_CHAIN->present();
  // bump the frame
  globals::TOTAL_NUMBER_OF_FRAMES += 1;
  globals::CURRENT_FRAME = (globals::CURRENT_FRAME + 1) % FRAME_BUFFERS_COUNT;
  return true;
}

RenderingContext *createDx12RenderingContext(
    const RenderingContextCreationSettings &settings, uint32_t width,
    uint32_t height) {
  auto *ctx = new Dx12RenderingContext(settings, width, height);
  dx12::RENDERING_CONTEXT = ctx;
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

  // initialize camera and light
  // ask for the camera buffer handle;
  m_cameraHandle =
      globals::CONSTANT_BUFFER_MANAGER->allocate(sizeof(CameraBuffer));

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

  // get the engine root signature to bind at the beginning of the frame
  engineRS = enginePerFrameEmptyRS("engineFrame");

  return result;
}

void Dx12RenderingContext::setupCameraForFrame() {
  globals::MAIN_CAMERA->updateCamera();
  // TODO fix this hardcoded parameter
  m_camBufferCPU.vFov = 60.0f;
  m_camBufferCPU.screenWidth =
      static_cast<float>(globals::ENGINE_CONFIG->m_windowWidth);
  m_camBufferCPU.screenHeight =
      static_cast<float>(globals::ENGINE_CONFIG->m_windowHeight);
  auto pos = globals::MAIN_CAMERA->getPosition();
  m_camBufferCPU.position = glm::vec4(pos, 1.0f);

  m_camBufferCPU.MVP =
      glm::transpose(globals::MAIN_CAMERA->getMVP(glm::mat4(1.0)));
  m_camBufferCPU.ViewMatrix =
      glm::transpose(globals::MAIN_CAMERA->getViewInverse(glm::mat4(1.0)));
  m_camBufferCPU.VPinverse =
      glm::transpose(globals::MAIN_CAMERA->getMVPInverse(glm::mat4(1.0)));
  m_camBufferCPU.perspectiveValues = globals::MAIN_CAMERA->getProjParams();

  globals::CONSTANT_BUFFER_MANAGER->update(m_cameraHandle, &m_camBufferCPU);
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;

  // let us bind the camera at the beginning of the frame
  auto commandList = currentFc->commandList;
  D3D12_GPU_DESCRIPTOR_HANDLE handle =
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(m_cameraHandle)
          .gpuHandle;
  commandList->SetGraphicsRootDescriptorTable(0, handle);
}

void Dx12RenderingContext::bindCameraBuffer(const int index) const {
  // assert(0);
  // TODO REMOVE
  // this code should not be called anymore and will need to remove after
  // transition of the whole multi-backend
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  D3D12_GPU_DESCRIPTOR_HANDLE handle =
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(m_cameraHandle)
          .gpuHandle;
  commandList->SetGraphicsRootDescriptorTable(0, handle);
}

void Dx12RenderingContext::bindCameraBufferCompute(const int index) const {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->SetComputeRootDescriptorTable(
      index,
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(m_cameraHandle)
          .gpuHandle);
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
  m_lightAABBHandle =
      dx12::DEBUG_RENDERER->drawAnimatedBoundingBoxFromFullPoints(
          m_lightAABBHandle, expanded, 1, glm::vec4(1, 0, 0, 1), "");

  // we can now use min max to generate the projection matrix needed;
  auto ortho =
      getOrthoMatrix(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ));
  m_light.projectionMatrix = glm::transpose(ortho);
  m_light.lightVP = glm::transpose(ortho * m_light.worldToLocal);
}

void Dx12RenderingContext::addRenderablesToQueue(const Renderable &renderable) {
  auto *typedQueues = reinterpret_cast<Dx12RenderingQueues *>(queues);

  Dx12Renderable dx12Renderable{};

  const Dx12MaterialRuntime &materialRuntime =
      dx12::MATERIAL_MANAGER->getMaterialRuntime(renderable.m_materialHandle);
  const Dx12MeshRuntime &meshRuntime =
      dx12::MESH_MANAGER->getMeshRuntime(renderable.m_meshHandle);

  dx12Renderable.m_materialRuntime = materialRuntime;
  dx12Renderable.m_meshRuntime = meshRuntime;
  // store the renderable on each queue
  for (int i = 0; i < MaterialManager::QUEUE_COUNT; ++i) {
    const uint32_t flag = materialRuntime.shaderQueueTypeFlags[i];

    if (flag != INVALID_QUEUE_TYPE_FLAGS) {
      (*typedQueues)[flag].emplace_back(dx12Renderable);
    }
  }
}

void Dx12RenderingContext::addRenderablesToQueue(
    const RenderableDescription &description) {
  auto *typedQueues = reinterpret_cast<Dx12RenderingQueues *>(queues);

  Dx12Renderable dx12Renderable{};

  const Dx12MaterialRuntime &materialRuntime =
      dx12::MATERIAL_MANAGER->getMaterialRuntime(description.materialHandle);

  Dx12MeshRuntime runtime{};
  runtime.bufferHandle = description.buffer;
  runtime.indexCount = description.primitiveToRender;
  runtime.positionRange = description.subranges[0];
  runtime.positionRange = description.subranges[0];
  runtime.normalsRange = description.subragesCount > 0
                             ? description.subranges[1]
                             : MemoryRange{0, 0};
  runtime.uvRange = description.subragesCount > 1 ? description.subranges[2]
                                                  : MemoryRange{0, 0};
  runtime.tangentsRange = description.subragesCount > 2
                              ? description.subranges[3]
                              : MemoryRange{0, 0};
  dx12Renderable.m_meshRuntime = runtime;
  dx12Renderable.m_materialRuntime = materialRuntime;

  // store the renderable on each queue
  for (int i = 0; i < MaterialManager::QUEUE_COUNT; ++i) {
    const uint32_t flag = materialRuntime.shaderQueueTypeFlags[i];

    if (flag != INVALID_QUEUE_TYPE_FLAGS) {
      (*typedQueues)[flag].emplace_back(dx12Renderable);
    }
  }
}

void Dx12RenderingContext::renderQueueType(const DrawCallConfig &config,
                                           const SHADER_QUEUE_FLAGS flag) {
  const auto &typedQueues = *(reinterpret_cast<Dx12RenderingQueues *>(queues));

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  for (const auto &renderableList : typedQueues) {
    if (dx12::MATERIAL_MANAGER->isQueueType(renderableList.first, flag)) {
      // now that we know the material goes in the the deferred queue we can
      // start rendering it

      // bind the corresponding RS and PSO
      dx12::MATERIAL_MANAGER->bindRSandPSO(renderableList.first, commandList);

      // this is most for debug, it will boil down to nothing in release
      const SHADER_TYPE_FLAGS type =
          dx12::MATERIAL_MANAGER->getTypeFlags(renderableList.first);
      const std::string &typeName =
          dx12::MATERIAL_MANAGER->getStringFromShaderTypeFlag(type);
      annotateGraphicsBegin(typeName.c_str());

      // looping each of the object
      const size_t count = renderableList.second.size();
      const Dx12Renderable *currRenderables = renderableList.second.data();
      for (size_t i = 0; i < count; ++i) {
        const Dx12Renderable &renderable = currRenderables[i];

        // bind material data like textures etc, then render
        dx12::MATERIAL_MANAGER->bindMaterial(flag, renderable.m_materialRuntime,
                                             commandList);

        // TODO temp check, needs to change
        bool isDebug = dx12::MATERIAL_MANAGER->isQueueType(
            renderableList.first, SHADER_QUEUE_FLAGS::DEBUG);
        dx12::MESH_MANAGER->render(renderable.m_meshRuntime, currentFc,
                                   !isDebug);
      }
      annotateGraphicsEnd();
    }
  }
}

void Dx12RenderingContext::renderMaterialType(const SHADER_QUEUE_FLAGS flag) {
  assert(0);
}

void Dx12RenderingContext::renderMesh(const MeshHandle handle, bool isIndexed) {
  // get mesh runtime
  const Dx12MeshRuntime runtime = dx12::MESH_MANAGER->getMeshRuntime(handle);
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  currentFc->commandList->IASetIndexBuffer(&runtime.iview);
  dx12::MESH_MANAGER->render(runtime, currentFc, isIndexed);
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

static const std::unordered_map<RESOURCE_STATE, D3D12_RESOURCE_STATES>
    RESOURCE_STATE_TO_DX_STATE = {
        {RESOURCE_STATE::GENERIC, D3D12_RESOURCE_STATE_COMMON},
        {RESOURCE_STATE::RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET},
        {RESOURCE_STATE::SHADER_READ_RESOURCE,
         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE},
        {RESOURCE_STATE::RANDOM_WRITE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS},
        {RESOURCE_STATE::DEPTH_RENDER_TARGET,
         D3D12_RESOURCE_STATE_DEPTH_WRITE}};

D3D12_RESOURCE_STATES toDx12ResourceState(RESOURCE_STATE state) {
  auto found = RESOURCE_STATE_TO_DX_STATE.find(state);
  if (found != RESOURCE_STATE_TO_DX_STATE.end()) {
    return found->second;
  }
  assert(
      0 &&
      "Could not find requested resource state for conversion to dx12 state");
  return D3D12_RESOURCE_STATE_COMMON;
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

    barrierCounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
        destination, toDx12ResourceState(binding.neededResourceState), barriers,
        barrierCounter);
    counter++;
  }

  // processing extra possible bindings, this are bindings that don't go as
  // output but possibly as inputs like dynamic resources that need to be
  // transitioned from write to read
  for (uint32_t i = 0; i < data.m_bindings.extraBindingsCount; ++i) {
    const RTBinding &binding = data.m_bindings.extraBindings[i];
    barrierCounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
        binding.handle, toDx12ResourceState(binding.neededResourceState),
        barriers, barrierCounter);
  }

  // processing the depth
  D3D12_CPU_DESCRIPTOR_HANDLE depthHandle = {0};
  if (data.m_bindings.depthStencil.handle.isHandleValid()) {
    const DepthBinding &binding = data.m_bindings.depthStencil;
    depthHandle = dx12::TEXTURE_MANAGER->getRTVDx12(binding.handle).cpuHandle;
    barrierCounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
        binding.handle, toDx12ResourceState(binding.neededResourceState),
        barriers, barrierCounter);
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

      globals::TEXTURE_MANAGER->clearRT(destination, &binding.clearColor.x);
    }
  }

  if (data.m_bindings.depthStencil.handle.isHandleValid()) {
    const DepthBinding &binding = data.m_bindings.depthStencil;
    if (binding.shouldClearDepth) {
      globals::TEXTURE_MANAGER->clearDepth(binding.handle,
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

void Dx12RenderingContext::clearBindingObject(
    const BufferBindingsHandle ) {
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
  resetAllocatorAndList(&dx12::CURRENT_FRAME_RESOURCE->fc);
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

  auto *heap = dx12::GLOBAL_CBV_SRV_UAV_HEAP->getResource();
  commandList->SetDescriptorHeaps(1, &heap);

  // let us bind the engine description: so we can bind the camera buffer
  commandList->SetGraphicsRootSignature(engineRS);

  return true;
}

bool Dx12RenderingContext::dispatchFrame() { return dispatchFrameDx12(); }

bool Dx12RenderingContext::resize(const uint32_t width, const uint32_t height) {
  return dx12::SWAP_CHAIN->resize(&dx12::CURRENT_FRAME_RESOURCE->fc, width,
                                  height);
}

bool Dx12RenderingContext::stopGraphic() { return stopGraphicsDx12(); }

bool Dx12RenderingContext::shutdownGraphic() { return shutdownGraphicsDx12(); }

void Dx12RenderingContext::flush() {
  flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
}

void Dx12RenderingContext::executeGlobalCommandList() {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  executeCommandList(dx12::GLOBAL_COMMAND_QUEUE, currentFc);
}

void Dx12RenderingContext::resetGlobalCommandList() {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  resetAllocatorAndList(currentFc);
}
}  // namespace SirEngine::dx12
