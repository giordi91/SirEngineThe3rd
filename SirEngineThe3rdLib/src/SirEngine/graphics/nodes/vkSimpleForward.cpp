#include "SirEngine/graphics/nodes/vkSimpleForward.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine {

static const char *SIMPLE_FORWARD_RS = "simpleMeshRSTex";
static const char *SIMPLE_FORWARD_PSO = "simpleMeshPSOTex";

VkSimpleForward::VkSimpleForward(GraphAllocators &allocators)
    : GNode("SimpleForward", "SimpleForward", allocators) {

  defaultInitializePlugsAndConnections(3, 1);
  // lets create the plugs
  GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";

  GPlug &depthBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depthTexture";

  GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";

  // fetching root signature
  // rs =
  //    dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SIMPLE_FORWARD_RS);
  // pso = dx12::PSO_MANAGER->getHandleFromName(SIMPLE_FORWARD_PSO);
  PSOHandle m_psoHandle = vk::PSO_MANAGER->getHandleFromName("forwardPhongPSO");
  m_pipeline = vk::PSO_MANAGER->getPipelineFromHandle(m_psoHandle);
  m_pass = vk::PSO_MANAGER->getRenderPassFromHandle(m_psoHandle);
}

void VkSimpleForward::initialize() {}

void VkSimpleForward::compute() {

  annotateGraphicsBegin("Simple Forward");

  // get input color texture
  const auto renderTarget =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  const auto depth =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);

  // draw calls go here
  VkViewport viewport{0,
                      float(globals::ENGINE_CONFIG->m_windowHeight),
                      float(globals::ENGINE_CONFIG->m_windowWidth),
                      -float(globals::ENGINE_CONFIG->m_windowHeight),
                      0.0f,
                      1.0f};
  VkRect2D scissor{
      {0, 0},
      {static_cast<uint32_t>(globals::ENGINE_CONFIG->m_windowWidth),
       static_cast<uint32_t>(globals::ENGINE_CONFIG->m_windowHeight)}};
  vkCmdSetViewport(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, 0, 1, &scissor);

  vkCmdBindPipeline(vk::CURRENT_FRAME_COMMAND->m_commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

  /*
  VkDescriptorSet sets[] = {m_meshDescriptorSet,
                            vk::STATIC_SAMPLER_DESCRIPTOR_SET};
  // multiple descriptor sets
  vkCmdBindDescriptorSets(vk::CURRENT_FRAME_COMMAND->m_commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, vk::PIPELINE_LAYOUT,
                          0, 2, sets, 0, nullptr);

  vk::MESH_MANAGER->renderMesh(meshHandle,
                               vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
                               */
}

void VkSimpleForward::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
