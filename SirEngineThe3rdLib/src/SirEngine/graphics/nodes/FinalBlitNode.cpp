
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/handle.h"
#include "WinPixEventRuntime/WinPixEventRuntime/pix3.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12MaterialManager.h"
#include "platform/windows/graphics/dx12/dx12SwapChain.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "platform/windows/graphics/dx12/dx12rootSignatureManager.h"

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

  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);

  // first we want to bind the back buffer so that we can render to it
  globals::TEXTURE_MANAGER->bindBackBuffer();
  // next we bind the material, this will among other things bind the pso and rs
  globals::MATERIAL_MANAGER->bindMaterial(m_matHandle);
  // we also need to bind the input resource, which is the texture we want to
  // blit
  globals::MATERIAL_MANAGER->bindTexture(m_matHandle, inputRTHandle, 1);
  // finally we submit a fullscreen pass
  globals::RENDERING_CONTEXT->fullScreenPass();

  // finishing the pass
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
}

void FinalBlitNode::initialize() {
  m_matHandle = globals::MATERIAL_MANAGER->allocateMaterial(
      "HDRtoSDREffect", "HDRtoSDREffect", 0);
}

void FinalBlitNode::populateNodePorts() {
  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  // empty binding since we bind to the swap chain buffer
  FrameBufferBindings bindings{};
  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  bindings.extraBindings = reinterpret_cast<RTBinding *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(RTBinding)));
  bindings.extraBindingsCount = 1;
  bindings.extraBindings[0].handle = inputRTHandle;
  bindings.extraBindings[0].currentResourceState = RESOURCE_STATE::GENERIC;
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
