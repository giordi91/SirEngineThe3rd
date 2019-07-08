#include "SirEngine/graphics/nodes/DebugNode.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/bufferManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "SirEngine/graphics/renderingContext.h"

namespace SirEngine {

static const std::string &GBUFFER_DEBUG_PSO_NAME = "gbufferDebugPSO";
static const std::string &NORMAL_DEBUG_PSO_NAME = "normalBufferDebugPSO";
static const std::string &SPECULAR_DEBUG_PSO_NAME = "specularBufferDebugPSO";
static const std::string &DEPTH_DEBUG_PSO_NAME = "depthBufferDebugPSO";
static const std::string &DEBUG_FULL_SCREEN_RS_NAME = "debugFullScreenBlit_RS";
static const std::string &DEBUG_REDUCE_DEPTH_RS_NANE = "depthMinMaxReduce_RS";
static const std::string &DEBUG_REDUCE_DEPTH_PSO_NAME = "depthMinMaxReduce_PSO";
static const std::string &DEBUG_REDUCE_DEPTH_CLEAR_PSO_NAME =
    "depthMinMaxReduceClear_PSO";

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
  depthReducePSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(DEBUG_REDUCE_DEPTH_PSO_NAME);
  depthReduceClearPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(DEBUG_REDUCE_DEPTH_CLEAR_PSO_NAME);

  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      (DEBUG_FULL_SCREEN_RS_NAME.c_str()));
  reduceRs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      (DEBUG_REDUCE_DEPTH_RS_NANE.c_str()));
}
void blitBuffer(const TextureHandle input, const TextureHandle handleToWriteOn,
                const PSOHandle psoHandle, ID3D12RootSignature *rs,
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
void blitDepthDebug(const TextureHandle input,
                    const TextureHandle handleToWriteOn,
                    const BufferHandle buffer, const PSOHandle psoHandle,
                    ID3D12RootSignature *rs,
                    const ConstantBufferHandle configHandle,
                    const BufferHandle reduceHandle) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[3];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      handleToWriteOn, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::BUFFER_MANAGER->transitionBufferIfNeeded(
      buffer, D3D12_RESOURCE_STATE_GENERIC_READ, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(handleToWriteOn, TextureHandle{});
  dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(input);

  commandList->SetGraphicsRootSignature(rs);

  // let us bind the reduce buffer
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(reduceHandle, 3, commandList);

  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      2,
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(configHandle)
          .gpuHandle);

  dx12::PSO_MANAGER->bindPSO(psoHandle, commandList);
  commandList->DrawInstanced(6, 1, 0, 0);
}

inline void checkHandle(const TextureHandle input,
                        const TextureHandle handleToWriteOn) {
  assert(input.isHandleValid());
  assert(input.handle != handleToWriteOn.handle);
}

void DebugNode::blitDebugFrame(const TextureHandle handleToWriteOn) {
  switch (m_index) {
  case (DebugIndex::GBUFFER): {
    TextureHandle input = globals::DEBUG_FRAME_DATA->geometryBuffer;
    checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, gbufferPSOHandle, rs,
               m_constBufferHandle);
    break;
  }
  case (DebugIndex::NORMAL_BUFFER): {
    TextureHandle input = globals::DEBUG_FRAME_DATA->normalBuffer;
    checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, normalPSOHandle, rs,
               m_constBufferHandle);
    break;
  }
  case (DebugIndex::SPECULAR_BUFFER): {
    TextureHandle input = globals::DEBUG_FRAME_DATA->specularBuffer;
    checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, specularPSOHandle, rs,
               m_constBufferHandle);
    break;
  }
  case (DebugIndex::GBUFFER_DEPTH): {
    TextureHandle input = globals::DEBUG_FRAME_DATA->gbufferDepth;
    checkHandle(input, handleToWriteOn);
    reduceDepth(globals::DEBUG_FRAME_DATA->gbufferDepth);
    blitDepthDebug(input, handleToWriteOn, m_reduceBufferHandle, depthPSOHandle,
                   rs, m_constBufferHandle, m_reduceBufferHandle);
    break;
  }
  default:
    assert(0 && "no valid pass to debug");
  }
}

void DebugNode::updateConstantBuffer() {
  globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferBuffered(
      m_constBufferHandle, &m_config);
  updateConfig = false;
}

void DebugNode::reduceDepth(const TextureHandle source) const {
  // we need to kick the compute shader
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[2];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      source, D3D12_RESOURCE_STATE_GENERIC_READ, barriers, counter);
  counter = dx12::BUFFER_MANAGER->transitionBufferIfNeeded(
      m_reduceBufferHandle, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barriers,
      counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  // lets kick the compute shaders
  commandList->SetComputeRootSignature(reduceRs);

  // binding the resources
  dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(source);
  // bind the source
  commandList->SetComputeRootDescriptorTable(1, pair.gpuHandle);
  // setup the
  // commandList->SetComputeRootDescriptorTable(
  //    2, dx12::CONSTANT_BUFFER_MANAGER
  //           ->getConstantBufferDx12Handle(m_textureConfigHandle)
  //           .gpuHandle);

  globals::RENDERING_CONTEX->bindCameraBufferCompute(2);
  dx12::BUFFER_MANAGER->bindBuffer(m_reduceBufferHandle, 0, commandList);
  // first we kick the clear
  dx12::PSO_MANAGER->bindPSO(depthReduceClearPSOHandle, commandList);
  commandList->Dispatch(1, 1, 1);

  // we need to issue a barrier here, both shaders write to the reduce buffer
  counter = 0;
  counter = dx12::BUFFER_MANAGER->bufferUAVTransition(m_reduceBufferHandle,
                                                      barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  dx12::PSO_MANAGER->bindPSO(depthReducePSOHandle, commandList);

  uint32_t width = globals::SCREEN_WIDTH;
  uint32_t height = globals::SCREEN_HEIGHT;
  uint32_t blockWidth = 64;
  uint32_t blockHeight = 4;
  uint32_t gx =
      width / blockWidth == 0 ? width / blockWidth : (width / blockWidth) + 1;
  uint32_t gy = height / blockHeight == 0 ? height / blockHeight
                                          : height / blockHeight + 1;
  commandList->Dispatch(gx, gy, 1);
}

void DebugNode::initialize() {
  m_constBufferHandle = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(DebugLayerConfig), &m_config);
  m_textureConfigHandle = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(TextureConfig), &m_textureConfig);

  m_reduceBufferHandle = globals::BUFFER_MANAGER->allocate(
      sizeof(ReducedDepth), nullptr, "depthDebugReduce", 1,
      sizeof(ReducedDepth), true);
}

void DebugNode::compute() {
  // get the render texture
  annotateGraphicsBegin("DebugPass");

  auto &conn = m_connections[&m_inputPlugs[0]];
  assert(conn.size() == 1 && "too many input connections");
  Plug *source = conn[0];
  TextureHandle texH{source->plugValue};

  // check if we need to update the constant buffer
  if (updateConfig) {
    updateConstantBuffer();
  }

#if SE_DEBUG
  blitDebugFrame(texH);
#endif
  m_outputPlugs[0].plugValue = texH.handle;
  annotateGraphicsEnd();
}
} // namespace SirEngine
