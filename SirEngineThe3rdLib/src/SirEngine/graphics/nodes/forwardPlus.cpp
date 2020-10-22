#include "SirEngine/graphics/nodes/forwardPlus.h"

#include "SirEngine/constantBufferManager.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/graphics/lightManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/log.h"
#include "SirEngine/textureManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine {

ForwardPlus::ForwardPlus(GraphAllocators& allocators)
    : GNode("ForwardPlus", "ForwardPlus", allocators) {
  defaultInitializePlugsAndConnections(0, 2);
  GPlug& outTexture = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";

  GPlug& depthBuffer = m_outputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";
}

void ForwardPlus::setupLight() {
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
  m_lightHandle = globals::LIGHT_MANAGER->addLight(
      m_light, graphics::LightManager::GPU_BACKED);

  // allocate the constant buffer
  m_lightCB = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(DirectionalLightData), 0, &m_light);

  graphics::BindingDescription descriptions[4] = {
      {0, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT},
      {1, GRAPHIC_RESOURCE_TYPE::TEXTURE,
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT},
      {2, GRAPHIC_RESOURCE_TYPE::TEXTURE,
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT},
      {3, GRAPHIC_RESOURCE_TYPE::TEXTURE,
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT}};
  m_passBindings = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      descriptions, 4,
      // graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "forwardPlusPassDataBindingTable");
}

void ForwardPlus::initialize(CommandBufferHandle commandBuffer,
                             RenderGraphContext* context) {
  initializeResolutionDepenantResources(commandBuffer, context);
  setupLight();
  uint32_t callbackCount = m_callbacks.size();
  for (uint32_t i = 0; i < callbackCount; ++i) {
    m_callbacks[i].callback->initialize(m_callbacks[i].id);
  }
}

void ForwardPlus::compute(RenderGraphContext* context) {
  // prepass callbacks
  uint32_t callbackCount = m_callbacks.size();
  for (uint32_t i = 0; i < callbackCount; ++i) {
    m_callbacks[i].callback->prePassRender(m_callbacks[i].id);
  }

  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);

  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(m_passBindings, m_lightCB,
                                                     0, 0);

  TextureHandle irradianceHandle =
      globals::RENDERING_CONTEXT->getEnviromentMapIrradianceHandle();
  globals::BINDING_TABLE_MANAGER->bindTexture(m_passBindings, irradianceHandle,
                                              1, 1, true);
  TextureHandle radianceHandle =
      globals::RENDERING_CONTEXT->getEnviromentMapRadianceHandle();
  globals::BINDING_TABLE_MANAGER->bindTexture(m_passBindings, radianceHandle, 2,
                                              2, true);
  TextureHandle brdfHandle = globals::RENDERING_CONTEXT->getBrdfHandle();
  globals::BINDING_TABLE_MANAGER->bindTexture(m_passBindings, brdfHandle, 3, 3,
                                              false);

  DrawCallConfig config{
      static_cast<uint32_t>(context->renderTargetWidth),
      static_cast<uint32_t>(context->renderTargetHeight), 0};
  globals::RENDERING_CONTEXT->renderQueueType(
      config, SHADER_QUEUE_FLAGS::FORWARD, m_passBindings);

  for (uint32_t i = 0; i < callbackCount; ++i) {
    m_callbacks[i].callback->passRender(m_callbacks[i].id, m_passBindings);
  }

  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
}

void ForwardPlus::onResizeEvent(int, int, CommandBufferHandle commandBuffer,
                                RenderGraphContext* context) {
  clearResolutionDepenantResources();
  initializeResolutionDepenantResources(commandBuffer, context);
}

void ForwardPlus::populateNodePorts(RenderGraphContext* context) {
  // setting the render target output handle
  m_outputPlugs[0].plugValue = m_rtHandle.handle;
  m_outputPlugs[1].plugValue = m_depthHandle.handle;

  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = m_rtHandle;
  bindings.colorRT[0].clearColor = {0.0, 0.0, 0.0, 1};
  bindings.colorRT[0].shouldClearColor = true;
  bindings.colorRT[0].currentResourceState =
      RESOURCE_STATE::SHADER_READ_RESOURCE;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].isSwapChainBackBuffer = 0;

  bindings.depthStencil.handle = m_depthHandle;
  // bindings.depthStencil.clearDepthColor= {1.0, 1.0, 1.0, 1.0};
  bindings.depthStencil.clearDepthColor = {0.0, 0.0, 0.0, 0.0};
  bindings.depthStencil.clearStencilColor = {0.0, 0.0, 0.0, 0.0};
  bindings.depthStencil.shouldClearDepth = true;
  bindings.depthStencil.shouldClearStencil = true;
  bindings.depthStencil.currentResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;
  bindings.depthStencil.neededResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;

  bindings.width = context->renderTargetWidth;
  bindings.height = context->renderTargetHeight;

  m_bindHandle =
      globals::RENDERING_CONTEXT->prepareBindingObject(bindings, "ForwardPlus");
}

void ForwardPlus::clear() {
  if (m_passBindings.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_passBindings);
  }
  uint32_t callbackCount = m_callbacks.size();
  for (uint32_t i = 0; i < callbackCount; ++i) {
    m_callbacks[i].callback->clear(m_callbacks[i].id);
  }
  clearResolutionDepenantResources();
}

void ForwardPlus::initializeResolutionDepenantResources(
    CommandBufferHandle, RenderGraphContext* context) {
  int width = context->renderTargetWidth;
  int height = context->renderTargetHeight;
  m_rtHandle = globals::TEXTURE_MANAGER->allocateTexture(
      width, height, RenderTargetFormat::R16G16B16A16_FLOAT, "simpleForwardRT",
      TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET |
          TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::SHADER_RESOURCE,
      RESOURCE_STATE::SHADER_READ_RESOURCE);
  m_depthHandle = globals::TEXTURE_MANAGER->allocateTexture(
      width, height, RenderTargetFormat::DEPTH_F32_S8, "simpleForwardDepth",
      TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::DEPTH_TEXTURE |
          TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::SHADER_RESOURCE,
      RESOURCE_STATE::DEPTH_RENDER_TARGET);

  uint32_t callbackCount = m_callbacks.size();
  for (uint32_t i = 0; i < callbackCount; ++i) {
    m_callbacks[i].callback->initializeResolutionDepenantResources(
        m_callbacks[i].id);
  }
}

void ForwardPlus::clearResolutionDepenantResources() {
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }

  if (m_rtHandle.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_rtHandle);
    m_rtHandle = {};
  }
  if (m_depthHandle.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_depthHandle);
    m_depthHandle = {};
  }

  uint32_t callbackCount = m_callbacks.size();
  for (uint32_t i = 0; i < callbackCount; ++i) {
    m_callbacks[i].callback->clearResolutionDepenantResources(
        m_callbacks[i].id);
  }
}
}  // namespace SirEngine
