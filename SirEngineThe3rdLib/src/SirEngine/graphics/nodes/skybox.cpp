#include "SirEngine/graphics/nodes/skybox.h"

#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"

namespace SirEngine {
static const char *SKYBOX_RS = "skyboxPSO";
static const char *SKYBOX_PSO = "skyboxPSO";

SkyBoxPass::SkyBoxPass(GraphAllocators &allocators)
    : GNode("SkyBoxPass", "SkyBoxPass", allocators) {
  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &fullscreenPass = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
  fullscreenPass.plugValue = 0;
  fullscreenPass.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  fullscreenPass.nodePtr = this;
  fullscreenPass.name = "fullscreenPass";

  GPlug &depthBuffer = m_inputPlugs[getPlugIndex(PLUGS::DEPTH)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";

  GPlug &buffer = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEX)];
  buffer.plugValue = 0;
  buffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
  buffer.nodePtr = this;
  buffer.name = "buffer";
}

void SkyBoxPass::initialize(CommandBufferHandle ,RenderGraphContext* ) {
  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(SKYBOX_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(SKYBOX_PSO);

  graphics::BindingDescription descriptions[1] = {
      {1, GRAPHIC_RESOURCE_TYPE::TEXTURE,
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT}};
  m_bindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      descriptions, 1, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_NONE,
      "skyboxBindingTable");
}

void SkyBoxPass::compute(RenderGraphContext* context) {
  // this will take care of binding the back buffer and the input and transition
  // both the back buffer  and input texture
  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);

  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_bindingTable, m_rs);

  globals::PSO_MANAGER->bindPSO(m_pso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_rs);

  // we clamp the viewport depth to the far plan. this means no matter how big
  // our sphere is it will be pushed to the far plane without artifacts:
  auto w = static_cast<float>(context->renderTargetWidth);
  auto h = static_cast<float>(context->renderTargetHeight);
  globals::RENDERING_CONTEXT->setViewportAndScissor(0, 0, w, h, 0, 0);
  //number vertices needed to render the cube
  globals::RENDERING_CONTEXT->renderProcedural(14);

  // finishing the pass
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
}

void SkyBoxPass::onResizeEvent(int, int,
                               const CommandBufferHandle commandBuffer,RenderGraphContext* context) {
  clearResolutionDepenantResources();
  initializeResolutionDepenantResources(commandBuffer,context);
}

void SkyBoxPass::populateNodePorts(RenderGraphContext* context) {
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

  bindings.width = context->renderTargetWidth;
  bindings.height = context->renderTargetHeight;

  m_bindHandle =
      globals::RENDERING_CONTEXT->prepareBindingObject(bindings, "Skybox");

  TextureHandle skyHandle =
      globals::RENDERING_CONTEXT->getEnviromentMapHandle();
  assert(skyHandle.isHandleValid());

  globals::BINDING_TABLE_MANAGER->bindTexture(m_bindingTable, skyHandle, 0, 1,
                                              true);
}

void SkyBoxPass::clear() {
  if (m_bindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_bindingTable);
  }
  clearResolutionDepenantResources();
}

void SkyBoxPass::clearResolutionDepenantResources()
{
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }
}
}  // namespace SirEngine
