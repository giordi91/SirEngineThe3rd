#include "SirEngine/graphics/nodes/gbufferPassPBR.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {
GBufferPassPBR::GBufferPassPBR(GraphAllocators &allocators)
    : GNode("GBufferPassPBR", "GBufferPassPBR", allocators) {

  defaultInitializePlugsAndConnections(1, 4);
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

  m_depth = dx12::TEXTURE_MANAGER->createDepthTexture(
      "gbufferDepth", globals::ENGINE_CONFIG->m_windowWidth, globals::ENGINE_CONFIG->m_windowHeight);

  m_geometryBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::ENGINE_CONFIG->m_windowWidth, globals::ENGINE_CONFIG->m_windowHeight, RenderTargetFormat::RGBA32,
      "geometryBuffer");

  m_normalBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::ENGINE_CONFIG->m_windowWidth, globals::ENGINE_CONFIG->m_windowHeight,
      RenderTargetFormat::R11G11B10_UNORM, "normalBuffer");

  m_specularBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::ENGINE_CONFIG->m_windowWidth, globals::ENGINE_CONFIG->m_windowHeight, RenderTargetFormat::RGBA32,
      "specularBuffer");
}

void GBufferPassPBR::compute() {

  annotateGraphicsBegin("GBufferPassPBR");

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  // first thing first we need to be able to bind the deferred buffers,
  // to do so we first transition them to be render targets
  D3D12_RESOURCE_BARRIER barriers[4];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_geometryBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_normalBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_specularBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  // now that they are in the right state we are going to clear them
  globals::TEXTURE_MANAGER->clearDepth(m_depth, 0.0f);
  float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  globals::TEXTURE_MANAGER->clearRT(m_geometryBuffer, color);
  globals::TEXTURE_MANAGER->clearRT(m_normalBuffer, color);
  globals::TEXTURE_MANAGER->clearRT(m_specularBuffer, color);

  // finally we can bind the buffers
  D3D12_CPU_DESCRIPTOR_HANDLE handles[3] = {
      dx12::TEXTURE_MANAGER->getRTVDx12(m_geometryBuffer).cpuHandle,
      dx12::TEXTURE_MANAGER->getRTVDx12(m_normalBuffer).cpuHandle,
      dx12::TEXTURE_MANAGER->getRTVDx12(m_specularBuffer).cpuHandle};

  auto depthDescriptor = dx12::TEXTURE_MANAGER->getRTVDx12(m_depth).cpuHandle;
  commandList->OMSetRenderTargets(3, handles, false, &depthDescriptor);

  // the stream is a series of rendarables sorted by type, so here we loop for
  // all the renderable types and filter for the one that are tagged for the
  // deferred queue
  globals::RENDERING_CONTEXT->renderQueueType(SHADER_QUEUE_FLAGS::DEFERRED);

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
  annotateGraphicsEnd();
}

inline void freeTextureIfValid(TextureHandle h) {
  if (h.isHandleValid()) {
    dx12::TEXTURE_MANAGER->free(h);
    h.handle = 0;
  }
}

void GBufferPassPBR::clear() {
  freeTextureIfValid(m_depth);
  freeTextureIfValid(m_geometryBuffer);
  freeTextureIfValid(m_normalBuffer);
  freeTextureIfValid(m_specularBuffer);
  m_generation =-1;
}

void GBufferPassPBR::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
