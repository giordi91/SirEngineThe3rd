#include "SirEngine/graphics/nodes/gbufferPassPBR.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/textureManager.h"

namespace SirEngine {
GBufferPassPBR::GBufferPassPBR(GraphAllocators &allocators)
    : GNode("GBufferPassPBR", "GBufferPassPBR", allocators) {

  defaultInitializePlugsAndConnections(0, 4);
  // lets create the plugs
  GPlug &geometryBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::GEOMETRY_RT)];
  geometryBuffer.plugValue = 0;
  geometryBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  geometryBuffer.nodePtr = this;
  geometryBuffer.name = "geometry";

  GPlug &normalBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::NORMALS_RT)];
  normalBuffer.plugValue = 0;
  normalBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  normalBuffer.nodePtr = this;
  normalBuffer.name = "normal";

  GPlug &specularBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::SPECULAR_RT)];
  specularBuffer.plugValue = 0;
  specularBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  specularBuffer.nodePtr = this;
  specularBuffer.name = "specular";

  GPlug &depthBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";
}

void GBufferPassPBR::initialize() {

  m_depth = globals::TEXTURE_MANAGER->allocateTexture(
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight, RenderTargetFormat::DEPTH_F32_S8,
      "gbufferDepth", TextureManager::TEXTURE_ALLOCATION_FLAGS::DEPTH_TEXTURE);

  m_geometryBuffer = globals::TEXTURE_MANAGER->allocateTexture(
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight, RenderTargetFormat::RGBA32,
      "geometryBuffer",
      TextureManager::TEXTURE_ALLOCATION_FLAGS::RENDER_TARGET);

  m_normalBuffer = globals::TEXTURE_MANAGER->allocateTexture(
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight,
      RenderTargetFormat::R11G11B10_UNORM, "normalBuffer",
      TextureManager::TEXTURE_ALLOCATION_FLAGS::RENDER_TARGET);

  m_specularBuffer = globals::TEXTURE_MANAGER->allocateTexture(
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight, RenderTargetFormat::RGBA32,
      "specularBuffer",
      TextureManager::TEXTURE_ALLOCATION_FLAGS::RENDER_TARGET);
}

void GBufferPassPBR::compute() {

  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);
  globals::RENDERING_CONTEXT->renderQueueType({}, SHADER_QUEUE_FLAGS::DEFERRED);
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
}

inline void freeTextureIfValid(TextureHandle h) {
  if (h.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(h);
    h.handle = 0;
  }
}

void GBufferPassPBR::clear() {
  freeTextureIfValid(m_depth);
  freeTextureIfValid(m_geometryBuffer);
  freeTextureIfValid(m_normalBuffer);
  freeTextureIfValid(m_specularBuffer);
  m_generation = -1;
}

void GBufferPassPBR::onResizeEvent(int, int) {
  clear();
  initialize();
}

void GBufferPassPBR::populateNodePorts() {
  // setting the data as output
  m_outputPlugs[0].plugValue = m_geometryBuffer.handle;
  m_outputPlugs[1].plugValue = m_normalBuffer.handle;
  m_outputPlugs[2].plugValue = m_specularBuffer.handle;
  m_outputPlugs[3].plugValue = m_depth.handle;

#if SE_DEBUG
  // if we are in debug we want to populate debug data such that can be
  // used for blitting debug data on screen
  globals::DEBUG_FRAME_DATA->geometryBuffer = m_geometryBuffer;
  globals::DEBUG_FRAME_DATA->normalBuffer = m_normalBuffer;
  globals::DEBUG_FRAME_DATA->specularBuffer = m_specularBuffer;
  globals::DEBUG_FRAME_DATA->gbufferDepth = m_depth;
#endif

  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = m_geometryBuffer;
  bindings.colorRT[0].clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  bindings.colorRT[0].shouldClearColor = true;
  bindings.colorRT[0].currentResourceState = RESOURCE_STATE::GENERIC;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;

  bindings.colorRT[1].handle = m_normalBuffer;
  bindings.colorRT[1].clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  bindings.colorRT[1].shouldClearColor = true;
  bindings.colorRT[1].currentResourceState = RESOURCE_STATE::GENERIC;
  bindings.colorRT[1].neededResourceState = RESOURCE_STATE::RENDER_TARGET;

  bindings.colorRT[2].handle = m_specularBuffer;
  bindings.colorRT[2].clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  bindings.colorRT[2].shouldClearColor = true;
  bindings.colorRT[2].currentResourceState = RESOURCE_STATE::GENERIC;
  bindings.colorRT[2].neededResourceState = RESOURCE_STATE::RENDER_TARGET;

  bindings.depthStencil.handle = m_depth;
  bindings.depthStencil.clearDepthColor = {0.0f, 0.0f, 0.0f, 1.0f};
  bindings.depthStencil.clearStencilColor = {0.0f, 0.0f, 0.0f, 1.0f};
  bindings.depthStencil.shouldClearDepth = true;
  bindings.depthStencil.shouldClearStencil = true;
  bindings.depthStencil.currentResourceState = RESOURCE_STATE::GENERIC;
  bindings.depthStencil.neededResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;

  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  m_bindHandle = globals::RENDERING_CONTEXT->prepareBindingObject(
      bindings, "gbufferPassPBR");
}
} // namespace SirEngine
