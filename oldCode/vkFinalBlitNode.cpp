
#include "SirEngine/graphics/nodes/vkFinalBlitNode.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/handle.h"

namespace SirEngine {

static const char *FINAL_BLIT_PSO = "HDRtoSDREffect_PSO";
static const char *FINAL_BLIT_RS = "standardPostProcessEffect_RS";
VkFinalBlitNode::VkFinalBlitNode(GraphAllocators &allocators)
    : GNode("VkFinalBlit", "VkFinalBlit", allocators) {
  // lets create the plugs
  defaultInitializePlugsAndConnections(1, 0);

  GPlug &inTexture= m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
}

void VkFinalBlitNode::compute() {
  // get the render texture
}

void VkFinalBlitNode::initialize() {
}

void VkFinalBlitNode::populateNodePorts()
{
    assert(0);
}
} // namespace SirEngine
