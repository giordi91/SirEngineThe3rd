#include "SirEngine/graphics/postProcess/postProcessStack.h"

#include "SirEngine/globals.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/handle.h"
#include "SirEngine/textureManager.h"

namespace SirEngine {

PostProcessStack::PostProcessStack(GraphAllocators &allocators)
    : GNode("PostProcessStack", "PostProcessStack", allocators) {
  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";

  GPlug &depthTexture = m_inputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
  depthTexture.plugValue = 0;
  depthTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  depthTexture.nodePtr = this;
  depthTexture.name = "depthTexture";

  GPlug &outTexture = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
}

void PostProcessStack::initialize(CommandBufferHandle commandBuffer,
                                  RenderGraphContext *context) {
  initializeResolutionDepenantResources(commandBuffer, context);
}

void PostProcessStack::compute() {
  if (m_stack.empty()) {
    return;
  }

  annotateGraphicsBegin("Post processing");

  m_internalCounter = 0;
  const size_t stackSize = m_stack.size();

  const PostProcessResources resources{inputDepthHandle};
  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandles[0]);
  m_stack[0]->render(inputRTHandle, handles[0], resources);
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandles[0]);

  for (size_t i = 1; i < stackSize; ++i) {
    const int previous = m_internalCounter;
    m_internalCounter = (m_internalCounter + 1) % 2;
    globals::RENDERING_CONTEXT->setBindingObject(
        m_bindHandles[m_internalCounter]);
    m_stack[i]->render(handles[previous], handles[m_internalCounter],
                       resources);
    globals::RENDERING_CONTEXT->clearBindingObject(
        m_bindHandles[m_internalCounter]);
  }
  annotateGraphicsEnd();
}

void PostProcessStack::clear() { clearResolutionDepenantResources(); }
void PostProcessStack::onResizeEvent(int, int,
                                     const CommandBufferHandle commandBuffer,
                                     RenderGraphContext *context) {
  clearResolutionDepenantResources();
  initializeResolutionDepenantResources(commandBuffer, context);
}

void PostProcessStack::populateNodePorts(RenderGraphContext *context) {
  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);
  inputDepthHandle =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);

  // we need to create the two set of bindings that we will use to ping pong the
  // buffers
  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = handles[0];
  bindings.colorRT[0].clearColor = {};
  bindings.colorRT[0].shouldClearColor = false;
  bindings.colorRT[0].currentResourceState =
      RESOURCE_STATE::SHADER_READ_RESOURCE;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].isSwapChainBackBuffer = 0;

  bindings.extraBindings = static_cast<RTBinding *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(RTBinding)));
  bindings.extraBindingsCount = 1;
  bindings.extraBindings[0].handle = inputRTHandle;
  bindings.extraBindings[0].currentResourceState =
      RESOURCE_STATE::RENDER_TARGET;
  bindings.extraBindings[0].neededResourceState =
      RESOURCE_STATE::SHADER_READ_RESOURCE;
  bindings.extraBindings[0].shouldClearColor = 0;

  bindings.depthStencil.handle = {};
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

  m_bindHandles[0] = globals::RENDERING_CONTEXT->prepareBindingObject(
      bindings, "postProcess1");

  // doing the second post process
  bindings.colorRT[0].handle = handles[1];
  bindings.extraBindings = nullptr;
  m_bindHandles[1] = globals::RENDERING_CONTEXT->prepareBindingObject(
      bindings, "postProcess2");

  // ports are populated statically at the beginning. this means that if we want
  // to bypass the node due to the stack being empty we need to do it here
  if (m_stack.empty()) {
    m_outputPlugs[0].plugValue = inputRTHandle.handle;
  } else {
    // if we have to output a buffer from our currently used stack then
    // we compute which one is going to be by simply iterating and computing the
    // index there might be a smarter way to compute the index but.... this
    // works so ...
    const size_t stackSize = m_stack.size();
    for (size_t i = 1; i < stackSize; ++i) {
      const int previous = m_internalCounter;
      m_internalCounter = (m_internalCounter + 1) % 2;
    }

    m_outputPlugs[0].plugValue = handles[m_internalCounter].handle;
  }
}

void PostProcessStack::initializeResolutionDepenantResources(
    CommandBufferHandle commandBuffer, RenderGraphContext *context) {
  // initialize all layers
  const size_t stackSize = m_stack.size();
  for (size_t i = 0; i < stackSize; ++i) {
    m_stack[i]->initialize();
  }

  // allocate ping pong textures
  handles[0] = globals::TEXTURE_MANAGER->allocateTexture(
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight,
      RenderTargetFormat::R16G16B16A16_FLOAT, "postProcess1",
      TextureManager::RENDER_TARGET | TextureManager::SHADER_RESOURCE,
      RESOURCE_STATE::SHADER_READ_RESOURCE);
  handles[1] = globals::TEXTURE_MANAGER->allocateTexture(
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight,
      RenderTargetFormat::R16G16B16A16_FLOAT, "postProcess2",
      TextureManager::RENDER_TARGET | TextureManager::SHADER_RESOURCE,
      RESOURCE_STATE::SHADER_READ_RESOURCE);
}

void PostProcessStack::clearResolutionDepenantResources() {
  if (handles[0].isHandleValid()) {
    globals::TEXTURE_MANAGER->free(handles[0]);
    handles[0].handle = 0;
  }
  if (handles[1].isHandleValid()) {
    globals::TEXTURE_MANAGER->free(handles[1]);
    handles[1].handle = 0;
  }

  if (m_bindHandles[0].isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandles[0]);
    m_bindHandles[0] = {};
  }
  if (m_bindHandles[1].isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandles[1]);
    m_bindHandles[1] = {};
  }
  // clearing the passes too
  for (auto *pass : m_stack) {
    pass->clear();
  }
}
}  // namespace SirEngine
