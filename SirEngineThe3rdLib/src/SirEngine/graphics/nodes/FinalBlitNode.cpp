
#include "SirEngine/graphics/nodes/FinalBlitNode.h"

#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/textureManager.h"

namespace SirEngine {

static const char *HDR_RS = "HDRtoSDREffect_PSO";
static const char *HDR_PSO = "HDRtoSDREffect_PSO";

FinalBlitNode::FinalBlitNode(GraphAllocators &allocators)
    : GNode("FinalBlit", "FinalBlit", allocators) {
  // lets create the plugs
  defaultInitializePlugsAndConnections(1, 0);

  GPlug &inTexture =
      m_inputPlugs[getPlugIndex(static_cast<uint32_t>(PLUGS::IN_TEXTURE))];
  inTexture.plugValue = 0;
  inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
}

void FinalBlitNode::compute(RenderGraphContext *context) {
  // this will take care of binding the back buffer and the input and transition
  // both the back buffer  and input texture
  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);

  globals::BINDING_TABLE_MANAGER->bindTexture(m_bindingTable, inputRTHandle, 0,
                                              0, false);
  globals::PSO_MANAGER->bindPSO(m_pso);
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_bindingTable, m_rs);

  // next we bind the material, this will among other things bind the pso and rs
  // finally we submit a fullscreen pass
  globals::RENDERING_CONTEXT->fullScreenPass();

  // finishing the pass
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
  // if handle is valid we are rendering to an offscreen buffer, as such we
  // transition to being sampled state
  if (context->m_renderTarget.isHandleValid()) {
    globals::TEXTURE_MANAGER->transitionTexture(
        {}, context->m_renderTarget, RESOURCE_STATE::RENDER_TARGET,
        RESOURCE_STATE::SHADER_READ_RESOURCE);
  }
}

void FinalBlitNode::initialize(CommandBufferHandle, RenderGraphContext *) {
  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(HDR_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(HDR_PSO);

  graphics::BindingDescription descriptions[1] = {
      {0, GRAPHIC_RESOURCE_TYPE::TEXTURE,
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT}};

  m_bindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      descriptions, 1,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "HDRtoSDREffect");
}

void FinalBlitNode::populateNodePorts(RenderGraphContext *context) {
  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  // empty binding since we bind to the swap chain buffer
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = context->m_renderTarget;
  bindings.colorRT[0].isSwapChainBackBuffer =
      !context->m_renderTarget.isHandleValid();
  bindings.colorRT[0].shouldClearColor = false;
  // if is not a swapchain backbuffer, the state is going to be different, we
  // would have bound the buffer for display so we transition it from shader read
  // to render target
  bindings.colorRT[0].currentResourceState =
      bindings.colorRT[0].isSwapChainBackBuffer
          ? RESOURCE_STATE::RENDER_TARGET
          : RESOURCE_STATE::SHADER_READ_RESOURCE;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.width = context->renderTargetWidth;
  bindings.height = context->renderTargetHeight;

  bindings.extraBindings = static_cast<RTBinding *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(RTBinding)));
  bindings.extraBindingsCount = 1;
  bindings.extraBindings[0].handle = inputRTHandle;
  bindings.extraBindings[0].currentResourceState =
      RESOURCE_STATE::RENDER_TARGET;
  bindings.extraBindings[0].neededResourceState =
      RESOURCE_STATE::SHADER_READ_RESOURCE;
  bindings.extraBindings[0].shouldClearColor = 0;

  m_bindHandle = globals::RENDERING_CONTEXT->prepareBindingObject(
      bindings, "EndOfFrameBlit");
}

void FinalBlitNode::clear() {
  if (m_bindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_bindingTable);
  }
  clearResolutionDepenantResources();
}

void FinalBlitNode::clearResolutionDepenantResources() {
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
    m_bindHandle = {};
  }
}

void FinalBlitNode::onResizeEvent(int, int,
                                  const CommandBufferHandle commandBuffer,
                                  RenderGraphContext *context) {
  clearResolutionDepenantResources();
  initializeResolutionDepenantResources(commandBuffer, context);
}
}  // namespace SirEngine
