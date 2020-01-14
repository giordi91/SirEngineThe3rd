#include "SirEngine/graphics/nodes/grassNode.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/dx12DebugRenderer.h"

namespace SirEngine {

GrassNode::GrassNode(GraphAllocators &allocators)
    : GNode("GrassNode", "GrassNode", allocators) {
  defaultInitializePlugsAndConnections(2, 2);
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
  GPlug &outDepth = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_DEPTH)];
  outDepth.plugValue = 0;
  outDepth.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outDepth.nodePtr = this;
  outDepth.name = "outDepth";
}

void GrassNode::initialize() {
  // lets read the grass file
  const char *grassFile = "../data/external/grass/points.json";
  auto jobj = getJsonObj(grassFile);

  // compute the amount of memory needed
  uint32_t tileCount = jobj.size();
  assert((tileCount > 0) && "no tiles found in the file");
  // assuming all tiles have same size
  assertInJson(jobj[0], "points");
  uint32_t pointCount = jobj[0]["points"].size();
  uint64_t totalSize = sizeof(float) * 3 * pointCount * tileCount;

  auto *data =
      reinterpret_cast<float *>(globals::FRAME_ALLOCATOR->allocate(totalSize));

  auto *aabbs = reinterpret_cast<BoundingBox *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(BoundingBox) * tileCount));

  // looping the tiles
  glm::vec3 zero{0, 0, 0};
  int counter = 0;
  int tileCounter = 0;
  for (const auto &tile : jobj) {
    assertInJson(tile, "aabb");
    assertInJson(tile, "points");
    const auto &aabbJ = tile["aabb"];
    const glm::vec3 minAABB(aabbJ[0][0].get<float>(), aabbJ[0][1].get<float>(),
                            aabbJ[0][2].get<float>());
    const glm::vec3 maxAABB(aabbJ[1][0].get<float>(), aabbJ[1][1].get<float>(),
                            aabbJ[1][2].get<float>());
    aabbs[tileCounter].min = minAABB;
    aabbs[tileCounter].max = maxAABB;

    const auto &points = tile["points"];
    uint32_t currentPointCount = points.size();
    assert(currentPointCount == pointCount);

    for (uint32_t i = 0; i < currentPointCount; ++i) {
      data[counter++] = points[i][0].get<float>();
      data[counter++] = points[i][1].get<float>();
      data[counter++] = points[i][2].get<float>();
    }
    tileCounter++;
  }

  m_debugHandle = globals::DEBUG_RENDERER->drawBoundingBoxes(
      aabbs, tileCount, glm::vec4(1, 0, 0, 1), "grassTiles");
}

void GrassNode::compute() {}

void GrassNode::onResizeEvent(int, int) {
  clear();
  initialize();
}

void GrassNode::populateNodePorts() {
  renderTarget = getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);
  depth = getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);

  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = renderTarget;
  bindings.colorRT[0].shouldClearColor = false;
  bindings.colorRT[0].currentResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].isSwapChainBackBuffer = 0;

  bindings.depthStencil.handle = depth;
  bindings.depthStencil.shouldClearDepth = false;
  bindings.depthStencil.shouldClearStencil = false;
  bindings.depthStencil.currentResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;
  bindings.depthStencil.neededResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;

  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  m_bindHandle =
      globals::RENDERING_CONTEXT->prepareBindingObject(bindings, "GrassPass");

  m_outputPlugs[0].plugValue = renderTarget.handle;
  m_outputPlugs[1].plugValue = depth.handle;
}

void GrassNode::clear() {
  if (m_debugHandle.isHandleValid()) {
    globals::DEBUG_RENDERER->free(m_debugHandle);
  }
  if(m_bindHandle.isHandleValid()) {
      globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }
}
}  // namespace SirEngine
