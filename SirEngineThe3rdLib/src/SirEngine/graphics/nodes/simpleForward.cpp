#include "SirEngine/graphics/nodes/simpleForward.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/debugRenderer.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

namespace SirEngine {

static const char *SIMPLE_FORWARD_RS = "simpleMeshRSTex";
static const char *SIMPLE_FORWARD_PSO = "simpleMeshPSOTex";

SimpleForward::SimpleForward(GraphAllocators &allocators)
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

  // lets create the plugs
  GPlug &stream = m_inputPlugs[PLUG_INDEX(PLUGS::ASSET_STREAM)];
  stream.plugValue = 0;
  stream.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
  stream.nodePtr = this;
  stream.name = "assetStream";

  GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";

  // fetching root signature
  rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SIMPLE_FORWARD_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(SIMPLE_FORWARD_PSO);
}

void SimpleForward::initialize() {
  m_brdfHandle = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/brdf.texture");
}

void SimpleForward::compute() {

  annotateGraphicsBegin("Simple Forward");

  // get input color texture
  const auto renderTarget =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  const auto depth =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);

  // we can now start to render our geometries, the way it works is you first
  // access the renderable stream coming in from the node input
  const StreamHandle streamH =
      getInputConnection<StreamHandle>(m_inConnections, ASSET_STREAM);
  const std::unordered_map<uint32_t, std::vector<Renderable>> &renderables =
      globals::ASSET_MANAGER->getRenderables(streamH);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[1];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(renderTarget, depth);
  // globals::TEXTURE_MANAGER->bindRenderTarget(renderTarget, TextureHandle{});

  for (const auto &renderableList : renderables) {
    if (dx12::MATERIAL_MANAGER->isQueueType(renderableList.first,
                                            SHADER_QUEUE_FLAGS::FORWARD)) {

      // now that we know the material goes in the the deferred queue we can
      // start rendering it

      // bind the corresponding RS and PSO
      dx12::MATERIAL_MANAGER->bindRSandPSO(renderableList.first, commandList);
      dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

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
        dx12::MATERIAL_MANAGER->bindMaterial(SHADER_QUEUE_FLAGS::FORWARD,renderable.m_materialHandle,
                                             commandList);
        dx12::MESH_MANAGER->bindMeshRuntimeAndRender(renderable.m_meshHandle,
                                                     currentFc);
      }
      annotateGraphicsEnd();
    }
  }

  m_outputPlugs[0].plugValue = renderTarget.handle;
  annotateGraphicsEnd();
}


void SimpleForward::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
