#include "SirEngine/graphics/nodes/simpleForward.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/debugRenderer.h"

namespace SirEngine {

static const char *SIMPLE_FORWARD_RS = "simpleMeshRSTex";
static const char *SIMPLE_FORWARD_PSO = "simpleMeshPSOTex";

SimpleForward::SimpleForward(const char *name)
    : GraphNode(name, "SimpleForward") {
  // lets create the plugs
  Plug inTexture;
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT| PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
  registerPlug(inTexture);

  Plug outTexture;
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
  registerPlug(outTexture);

  Plug depthBuffer;
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT| PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depthTexture";
  registerPlug(depthBuffer);

  // lets create the plugs
  Plug stream;
  stream.plugValue = 0;
  stream.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
  stream.nodePtr = this;
  stream.name = "assetStream";
  registerPlug(stream);


  // fetching root signature
  rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SIMPLE_FORWARD_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(SIMPLE_FORWARD_PSO);
}

void SimpleForward::initialize() {
  m_brdfHandle = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/brdf.texture");


}
inline StreamHandle
getInputConnection(std::unordered_map<const Plug *, std::vector<Plug *>> &conns,
                   Plug *plug) {
  // TODO not super safe to do this, might be worth improving this
  auto &inConns = conns[plug];
  assert(inConns.size() == 1 && "too many input connections");
  Plug *source = inConns[0];
  const auto h = StreamHandle{source->plugValue};
  assert(h.isHandleValid());
  return h;
}

void SimpleForward::compute() {

  annotateGraphicsBegin("Simple Forward");

  //get input color texture
  auto &conn = m_connections[&m_inputPlugs[0]];
  assert(conn.size() == 1 && "too many input connections");
  Plug *source = conn[0];
  const TextureHandle renderTarget{source->plugValue};

  auto &connDepth = m_connections[&m_inputPlugs[1]];
  assert(connDepth.size() == 1 && "too many input connections");
  Plug *sourceDepth = connDepth[0];
  const TextureHandle depth{sourceDepth->plugValue};

  // we can now start to render our geometries, the way it works is you first
  // access the renderable stream coming in from the node input
  const StreamHandle streamH =
      getInputConnection(m_connections, &m_inputPlugs[2]);
  const std::unordered_map<uint32_t, std::vector<Renderable>> &renderables =
      globals::ASSET_MANAGER->getRenderables(streamH);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[1];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers,
      counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(renderTarget, depth);
  //globals::TEXTURE_MANAGER->bindRenderTarget(renderTarget, TextureHandle{});

  for (const auto &renderableList : renderables) {
    if (dx12::MATERIAL_MANAGER->isQueueType(renderableList.first,
                                            SHADER_QUEUE_FLAGS::FORWARD)) {

      // now that we know the material goes in the the deferred queue we can
      // start rendering it

      // bind the corresponding RS and PSO
      dx12::MATERIAL_MANAGER->bindRSandPSO(renderableList.first, commandList);
      globals::RENDERING_CONTEXT->bindCameraBuffer(0);

      // this is most for debug, it will boil down to nothing in release
      const SHADER_TYPE_FLAGS type =
          dx12::MATERIAL_MANAGER->getTypeFlags(renderableList.first);
      const std::string &typeName =
          dx12::MATERIAL_MANAGER->getStringFromShaderTypeFlag(type);
      annotateGraphicsBegin(typeName.c_str());

      // looping each of the object
      const size_t count = renderableList.second.size();
      const Renderable *currRenderables = renderableList.second.data();
      for (int i = 0; i < count; ++i) {
        const Renderable &renderable = currRenderables[i];

        // bind material data like textures etc, then render
        dx12::MATERIAL_MANAGER->bindMaterial(renderable.m_materialRuntime,
                                             commandList);
        dx12::MESH_MANAGER->bindMeshRuntimeAndRender(renderable.m_meshRuntime,
                                                     currentFc);
      }
      annotateGraphicsEnd();

    }
  }
  //TEMP
  dx12::DEBUG_RENDERER->render();

  m_outputPlugs[0].plugValue = renderTarget.handle;
  annotateGraphicsEnd();
}

void SimpleForward::clear() {
}

void SimpleForward::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
