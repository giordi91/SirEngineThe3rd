#include "SirEngine/graphics/nodes/DebugNode.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

namespace SirEngine {

DebugNode::DebugNode(const char *name) : GraphNode(name, "DebugNode") {
  // lets create the plugs
  Plug inTexture;
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
  registerPlug(inTexture);

  Plug outTexture;
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
  registerPlug(outTexture);
}

void blitGBuffeer(const TextureHandle handleToWriteOn) {
  // we need the shader to extract the information of the gbuffer
  ID3D12PipelineState *pso =
      dx12::PSO_MANAGER->getComputePSOByName("gbufferDebugPSO");
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          ("fullScreenPassRS"));
}

void DebugNode::blitDebugFrame(const TextureHandle handleToWriteOn) {
  switch (m_index) {
  case (DebugIndex::GBUFFER): {
    blitGBuffeer(handleToWriteOn);
  }
  }
}

void DebugNode::compute() {
  // get the render texture

  auto &conn = m_connections[&m_inputPlugs[0]];
  assert(conn.size() == 1 && "too many input connections");
  Plug *source = conn[0];
  TextureHandle texH;
  texH.handle = source->plugValue;

#if SE_DEBUG
  blitDebugFrame(texH);
#else
  m_outputPlugs[0].plugValue = texH.handle;
#endif
}
} // namespace SirEngine
