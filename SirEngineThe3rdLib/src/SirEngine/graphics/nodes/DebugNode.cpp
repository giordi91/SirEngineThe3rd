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
void blitBuffer(const TextureHandle input, const TextureHandle handleToWriteOn,
                ID3D12PipelineState *pso, ID3D12RootSignature *rs) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[2];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      handleToWriteOn, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(handleToWriteOn, TextureHandle{});
  dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(input);

  commandList->SetPipelineState(pso);
  commandList->SetGraphicsRootSignature(rs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->DrawInstanced(6, 1, 0, 0);
}

void blitGBuffeer(const TextureHandle handleToWriteOn) {
  // we need the shader to extract the information of the gbuffer
  ID3D12PipelineState *pso =
      dx12::PSO_MANAGER->getComputePSOByName("gbufferDebugPSO");
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          ("genericFullScreenBlit_RS"));

  TextureHandle input = globals::DEBUG_FRAME_DATA->geometryBuffer;
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
  blitBuffer(input, handleToWriteOn, pso, rs);
}
void blitNormalBuffer(const TextureHandle handleToWriteOn) {
  // we need the shader to extract the information of the gbuffer
  ID3D12PipelineState *pso =
      dx12::PSO_MANAGER->getComputePSOByName("normalBufferDebugPSO");
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          ("genericFullScreenBlit_RS"));
  TextureHandle input = globals::DEBUG_FRAME_DATA->normalBuffer;
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
  blitBuffer(input, handleToWriteOn, pso, rs);
}
void blitSpecularBuffer(const TextureHandle handleToWriteOn) {
  // we need the shader to extract the information of the gbuffer
  ID3D12PipelineState *pso =
      dx12::PSO_MANAGER->getComputePSOByName("specularBufferDebugPSO");
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          ("genericFullScreenBlit_RS"));
  TextureHandle input = globals::DEBUG_FRAME_DATA->specularBuffer;
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
  blitBuffer(input, handleToWriteOn, pso, rs);
}
void blitDepthBuffer(const TextureHandle handleToWriteOn) {
  // we need the shader to extract the information of the gbuffer
  ID3D12PipelineState *pso =
      dx12::PSO_MANAGER->getComputePSOByName("depthBufferDebugPSO");
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          ("genericFullScreenBlit_RS"));
  TextureHandle input = globals::DEBUG_FRAME_DATA->gbufferDepth;
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
  blitBuffer(input, handleToWriteOn, pso, rs);
}

void DebugNode::blitDebugFrame(const TextureHandle handleToWriteOn) {
  switch (m_index) {
  case (DebugIndex::GBUFFER): {
    blitGBuffeer(handleToWriteOn);
    break;
  }
  case (DebugIndex::NORMAL_BUFFER): {
    blitNormalBuffer(handleToWriteOn);
    break;
  }
  case (DebugIndex::SPECULAR_BUFFER): {
    blitSpecularBuffer(handleToWriteOn);
    break;
  }
  case (DebugIndex::GBUFFER_DEPTH): {
    blitDepthBuffer(handleToWriteOn);
    break;
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
  m_outputPlugs[0].plugValue = texH.handle;
#else
  m_outputPlugs[0].plugValue = texH.handle;
#endif
}
} // namespace SirEngine
