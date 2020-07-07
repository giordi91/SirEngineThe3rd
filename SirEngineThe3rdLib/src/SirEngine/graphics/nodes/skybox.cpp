#include "SirEngine/graphics/nodes/skybox.h"

#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"

namespace SirEngine {
static const char *SKYBOX_RS = "skybox_RS";
static const char *SKYBOX_PSO = "skyboxPSO";

SkyBoxPass::SkyBoxPass(GraphAllocators &allocators)
    : GNode("SkyBoxPass", "SkyBoxPass", allocators) {
  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &fullscreenPass = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  fullscreenPass.plugValue = 0;
  fullscreenPass.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  fullscreenPass.nodePtr = this;
  fullscreenPass.name = "fullscreenPass";

  GPlug &depthBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";

  GPlug &buffer = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEX)];
  buffer.plugValue = 0;
  buffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  buffer.nodePtr = this;
  buffer.name = "buffer";
}

void SkyBoxPass::initialize() {
  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(SKYBOX_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(SKYBOX_PSO);

  skyboxHandle = globals::MESH_MANAGER->loadMesh(
      "../data/processed/meshes/skybox.model", true);
  //"../data/processed/meshes/knightB/jacket.model", true);

  const char *queues[5] = {nullptr, nullptr, nullptr, nullptr, "skybox"};
  m_matHandle = globals::MATERIAL_MANAGER->allocateMaterial("skybox", 0, queues

  );
}

void SkyBoxPass::compute() {
  // this will take care of binding the back buffer and the input and transition
  // both the back buffer  and input texture
  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);

  // next we bind the material, this will among other things bind the pso and rs
  globals::MATERIAL_MANAGER->bindMaterial(m_matHandle,
                                          SHADER_QUEUE_FLAGS::CUSTOM);

  // we clamp the viewport depth to the far plan. this means no matter how big
  // our sphere is it will be pushed to the far plane without artifacts:
  auto w = static_cast<float>(globals::ENGINE_CONFIG->m_windowWidth);
  auto h = static_cast<float>(globals::ENGINE_CONFIG->m_windowHeight);
  globals::RENDERING_CONTEXT->setViewportAndScissor(0, 0, w, h, 0, 0);

  globals::RENDERING_CONTEXT->bindCameraBuffer(0);
  globals::RENDERING_CONTEXT->renderMesh(skyboxHandle, true);

  // finishing the pass
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
}

void SkyBoxPass::onResizeEvent(int, int) {
  clear();
  initialize();
}

void SkyBoxPass::populateNodePorts() {
  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);
  inputDepthHandle = getInputConnection<TextureHandle>(m_inConnections, DEPTH);

  // setting the render target output handle
  m_outputPlugs[0].plugValue = inputRTHandle.handle;

  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = inputRTHandle;
  bindings.colorRT[0].clearColor = {};
  bindings.colorRT[0].shouldClearColor = false;
  bindings.colorRT[0].currentResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].isSwapChainBackBuffer = 0;

  bindings.depthStencil.handle = inputDepthHandle;
  bindings.depthStencil.clearDepthColor = {};
  bindings.depthStencil.clearStencilColor = {};
  bindings.depthStencil.shouldClearDepth = false;
  bindings.depthStencil.shouldClearStencil = false;
  bindings.depthStencil.currentResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;
  bindings.depthStencil.neededResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;

  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  m_bindHandle =
      globals::RENDERING_CONTEXT->prepareBindingObject(bindings, "Skybox");

  TextureHandle skyHandle =
      globals::RENDERING_CONTEXT->getEnviromentMapHandle();
  assert(skyHandle.isHandleValid());
  globals::MATERIAL_MANAGER->bindTexture(m_matHandle, skyHandle, 0, 1,
                                         SHADER_QUEUE_FLAGS::CUSTOM, true);
  globals::MATERIAL_MANAGER->bindMesh(m_matHandle, skyboxHandle, 1, 0,
                                      MeshAttributeFlags::POSITIONS,
                                      SHADER_QUEUE_FLAGS::CUSTOM);
}

void SkyBoxPass::clear() {
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }
}
}  // namespace SirEngine
