
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"

namespace SirEngine {

FinalBlitNode::FinalBlitNode(GraphAllocators &allocators)
    : GNode("FinalBlit", "FinalBlit", allocators) {
  // lets create the plugs
  defaultInitializePlugsAndConnections(1, 0);

  GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
}

void FinalBlitNode::compute() {

  // this will take care of binding the back buffer and the input and transition
  // both the back buffer  and input texture
  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);
  // next we bind the material, this will among other things bind the pso and rs
  globals::MATERIAL_MANAGER->bindMaterial(m_matHandle);
  // we also need to bind the input resource, which is the texture we want to
  // blit
  static int test =0;
  if(test ==0) {
  
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
  globals::MATERIAL_MANAGER->bindTexture(m_matHandle, inputRTHandle, 1);
  } else {
  globals::MATERIAL_MANAGER->bindTexture(m_matHandle, inputRTHandle, 0);
  }
  }

  test++;
  // finally we submit a fullscreen pass
  globals::RENDERING_CONTEXT->fullScreenPass();

  // finishing the pass
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
}

void FinalBlitNode::initialize() {
  // TODO temp hack
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
    m_matHandle = globals::MATERIAL_MANAGER->allocateMaterial(
        "HDRtoSDREffect", "HDRtoSDREffect", 0);
  } else {
    m_matHandle = globals::MATERIAL_MANAGER->allocateMaterial(
        "vkHDRtoSDREffect", "HDRtoSDREffect", 0);
  }
}

void FinalBlitNode::populateNodePorts() {
  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  // empty binding since we bind to the swap chain buffer
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = {};
  bindings.colorRT[0].isSwapChainBackBuffer = true;
  bindings.colorRT[0].shouldClearColor = false;
  bindings.colorRT[0].currentResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  bindings.extraBindings = reinterpret_cast<RTBinding *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(RTBinding)));
  bindings.extraBindingsCount = 1;
  bindings.extraBindings[0].handle = inputRTHandle;
  bindings.extraBindings[0].currentResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.extraBindings[0].neededResourceState =
      RESOURCE_STATE::SHADER_READ_RESOURCE;
  bindings.extraBindings[0].shouldClearColor = 0;

  m_bindHandle = globals::RENDERING_CONTEXT->prepareBindingObject(
      bindings, "EndOfFrameBlit");
}

void FinalBlitNode::clear() {
  // TODO why do i need the guard?, clear somehow gets called before
  // the initialize and populateNodesPorts
  if (m_matHandle.isHandleValid()) {
    globals::MATERIAL_MANAGER->free(m_matHandle);
  }
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }
}
} // namespace SirEngine
