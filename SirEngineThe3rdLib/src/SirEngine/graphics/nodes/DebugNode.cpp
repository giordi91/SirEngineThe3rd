#include "SirEngine/graphics/nodes/DebugNode.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

namespace SirEngine {

static const std::string &GBUFFER_DEBUG_PSO_NAME = "gbufferDebugPSO";
static const std::string &NORMAL_DEBUG_PSO_NAME = "normalBufferDebugPSO";
static const std::string &SPECULAR_DEBUG_PSO_NAME = "specularBufferDebugPSO";
static const std::string &DEPTH_DEBUG_PSO_NAME = "depthBufferDebugPSO";
static const std::string &DEBUG_FULL_SCREEN_RS = "debugFullScreenBlit_RS";

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

  gbufferPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(GBUFFER_DEBUG_PSO_NAME);
  normalPSOHandle = dx12::PSO_MANAGER->getHandleFromName(NORMAL_DEBUG_PSO_NAME);
  specularPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(SPECULAR_DEBUG_PSO_NAME);
  depthPSOHandle = dx12::PSO_MANAGER->getHandleFromName(DEPTH_DEBUG_PSO_NAME);

  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      (DEBUG_FULL_SCREEN_RS.c_str()));
}
void blitBuffer(const TextureHandle input, const TextureHandle handleToWriteOn,
                PSOHandle psoHandle, ID3D12RootSignature *rs,
                const ConstantBufferHandle configHandle) {
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

  // commandList->SetPipelineState(pso);
  dx12::PSO_MANAGER->bindPSO(psoHandle, commandList);
  commandList->SetGraphicsRootSignature(rs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);

  commandList->SetGraphicsRootDescriptorTable(
      2,
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(configHandle)
          .gpuHandle);

  commandList->DrawInstanced(6, 1, 0, 0);
}
void blitBuffer(const TextureHandle input, const TextureHandle handleToWriteOn,
                ID3D12PipelineState *pso, ID3D12RootSignature *rs,
                const ConstantBufferHandle configHandle) {
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

  commandList->SetGraphicsRootDescriptorTable(
      2,
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(configHandle)
          .gpuHandle);

  commandList->DrawInstanced(6, 1, 0, 0);
}

void blitGBuffeer(const TextureHandle handleToWriteOn,
                  const ConstantBufferHandle configHandle,
                  PSOHandle psoHandle) {
  // we need the shader to extract the information of the gbuffer
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          (DEBUG_FULL_SCREEN_RS.c_str()));

  TextureHandle input = globals::DEBUG_FRAME_DATA->geometryBuffer;
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
  // blitBuffer(input, handleToWriteOn, pso, rs, configHandle);
  blitBuffer(input, handleToWriteOn, psoHandle, rs, configHandle);
}
void blitNormalBuffer(const TextureHandle handleToWriteOn,
                      const ConstantBufferHandle configHandle) {
  // we need the shader to extract the information of the gbuffer
  ID3D12PipelineState *pso =
      dx12::PSO_MANAGER->getComputePSOByName(NORMAL_DEBUG_PSO_NAME);
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          DEBUG_FULL_SCREEN_RS.c_str());
  TextureHandle input = globals::DEBUG_FRAME_DATA->normalBuffer;
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
  blitBuffer(input, handleToWriteOn, pso, rs, configHandle);
}
void blitSpecularBuffer(const TextureHandle handleToWriteOn,
                        const ConstantBufferHandle configHandle) {
  // we need the shader to extract the information of the gbuffer
  ID3D12PipelineState *pso =
      dx12::PSO_MANAGER->getComputePSOByName(SPECULAR_DEBUG_PSO_NAME);
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          DEBUG_FULL_SCREEN_RS.c_str());
  TextureHandle input = globals::DEBUG_FRAME_DATA->specularBuffer;
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
  blitBuffer(input, handleToWriteOn, pso, rs, configHandle);
}
void blitDepthBuffer(const TextureHandle handleToWriteOn,
                     const ConstantBufferHandle configHandle) {
  // we need the shader to extract the information of the gbuffer
  ID3D12PipelineState *pso =
      dx12::PSO_MANAGER->getComputePSOByName(DEPTH_DEBUG_PSO_NAME);
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
          DEBUG_FULL_SCREEN_RS.c_str());
  TextureHandle input = globals::DEBUG_FRAME_DATA->gbufferDepth;
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
  blitBuffer(input, handleToWriteOn, pso, rs, configHandle);
}

inline void checkHandle(TextureHandle input, TextureHandle handleToWriteOn) {
    assert(input.isHandleValid());
    assert(input.handle != handleToWriteOn.handle);
}

void DebugNode::blitDebugFrame(const TextureHandle handleToWriteOn) {
  switch (m_index) {
  case (DebugIndex::GBUFFER): {
    TextureHandle input = globals::DEBUG_FRAME_DATA->geometryBuffer;
	checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, gbufferPSOHandle, rs, m_constBufferHandle);
    break;
  }
  case (DebugIndex::NORMAL_BUFFER): {
    TextureHandle input = globals::DEBUG_FRAME_DATA->normalBuffer;
	checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, normalPSOHandle, rs, m_constBufferHandle);
    break;
  }
  case (DebugIndex::SPECULAR_BUFFER): {
    TextureHandle input = globals::DEBUG_FRAME_DATA->specularBuffer;
	checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, specularPSOHandle, rs, m_constBufferHandle);
    break;
  }
  case (DebugIndex::GBUFFER_DEPTH): {
    TextureHandle input = globals::DEBUG_FRAME_DATA->gbufferDepth;
	checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, depthPSOHandle, rs, m_constBufferHandle);
    break;
  }
  }
}

void DebugNode::updateConstantBuffer() {

  globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferBuffered(
      m_constBufferHandle, &m_config);
  updateConfig = false;
}

void DebugNode::initialize() {
  m_constBufferHandle = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(DebugLayerConfig), &m_config);
}

void DebugNode::compute() {
  // get the render texture

  auto &conn = m_connections[&m_inputPlugs[0]];
  assert(conn.size() == 1 && "too many input connections");
  Plug *source = conn[0];
  TextureHandle texH;
  texH.handle = source->plugValue;

  // check if we need to update the constant buffer
  if (updateConfig) {
    updateConstantBuffer();
  }

#if SE_DEBUG
  blitDebugFrame(texH);
#endif
  m_outputPlugs[0].plugValue = texH.handle;
}
} // namespace SirEngine
