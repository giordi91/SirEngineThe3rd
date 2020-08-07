#include "SirEngine/graphics/nodes/framePassDebugNode.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12BufferManager.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"

namespace SirEngine {

static const char *GBUFFER_DEBUG_PSO_NAME = "gbufferDebugPSO";
static const char *NORMAL_DEBUG_PSO_NAME = "normalBufferDebugPSO";
static const char *METALLIC_DEBUG_PSO_NAME = "metallicBufferDebugPSO";
static const char *ROUGHNESS_DEBUG_PSO_NAME = "roughnessBufferDebugPSO";
static const char *THICKNESS_DEBUG_PSO_NAME = "thicknessBufferDebugPSO";
static const char *STENCIL_DEBUG_PSO_NAME = "stencilDebugPSO";
static const char *DEPTH_DEBUG_PSO_NAME = "depthBufferDebugPSO";
static const char *DEBUG_FULL_SCREEN_RS_NAME = "debugFullScreenBlit_RS";
static const char *DEBUG_REDUCE_DEPTH_RS_NAME = "depthMinMaxReduce_RS";
static const char *DEBUG_REDUCE_DEPTH_PSO_NAME = "depthMinMaxReduce_PSO";
static const char *DEBUG_REDUCE_DEPTH_CLEAR_PSO_NAME =
    "depthMinMaxReduceClear_PSO";

FramePassDebugNode::FramePassDebugNode(GraphAllocators &allocators)
    : GNode("FramePassDebugNode", "FramePassDebugNode", allocators) {
  defaultInitializePlugsAndConnections(1, 1);
  // lets create the plugs
  GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";

  GPlug &outTexture = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";

  m_gbufferPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(GBUFFER_DEBUG_PSO_NAME);
  m_normalPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(NORMAL_DEBUG_PSO_NAME);
  m_metallicPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(METALLIC_DEBUG_PSO_NAME);
  m_roughnessPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(ROUGHNESS_DEBUG_PSO_NAME);
  m_thicknessPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(THICKNESS_DEBUG_PSO_NAME);
  m_stencilPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(STENCIL_DEBUG_PSO_NAME);
  m_depthPSOHandle = dx12::PSO_MANAGER->getHandleFromName(DEPTH_DEBUG_PSO_NAME);
  m_depthReducePSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(DEBUG_REDUCE_DEPTH_PSO_NAME);
  m_depthReduceClearPSOHandle =
      dx12::PSO_MANAGER->getHandleFromName(DEBUG_REDUCE_DEPTH_CLEAR_PSO_NAME);

  m_rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      (DEBUG_FULL_SCREEN_RS_NAME));
  m_reduceRs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      (DEBUG_REDUCE_DEPTH_RS_NAME));
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
void blitStencilDebug(const TextureHandle input,
                      const TextureHandle handleToWriteOn,
                      const PSOHandle psoHandle, ID3D12RootSignature *rs,
                      const DebugLayerConfig &cpuConfig) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  // TODO the input should be the depth, there should be
  // no need for us to get it directly
  // TextureHandle depth = globals::DEBUG_FRAME_DATA->gbufferDepth;
  const TextureHandle depth = input;
  D3D12_RESOURCE_BARRIER barriers[2];
  int counter = 0;
  float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  globals::TEXTURE_MANAGER->clearRT(handleToWriteOn, black);

  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      handleToWriteOn, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(handleToWriteOn, depth);

  // commandList->SetPipelineState(pso);
  dx12::PSO_MANAGER->bindPSO(psoHandle, commandList);
  commandList->SetGraphicsRootSignature(rs);
  commandList->OMSetStencilRef(cpuConfig.stencilValue);
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
      buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(handleToWriteOn, TextureHandle{});
  const dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(input);

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

void FramePassDebugNode::populateNodePorts() {
  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);
  m_outputPlugs[0].plugValue = inputRTHandle.handle;
}

void FramePassDebugNode::blitDebugFrame(
    const TextureHandle handleToWriteOn) const {
  switch (m_index) {
  case (DebugIndex::GBUFFER): {
    const TextureHandle input = globals::DEBUG_FRAME_DATA->geometryBuffer;
    checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, m_gbufferPSOHandle, m_rs,
               m_constBufferHandle);
    break;
  }
  case (DebugIndex::NORMAL_BUFFER): {
    const TextureHandle input = globals::DEBUG_FRAME_DATA->normalBuffer;
    checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, m_normalPSOHandle, m_rs,
               m_constBufferHandle);
    break;
  }
  case (DebugIndex::METALLIC_BUFFER): {
    const TextureHandle input = globals::DEBUG_FRAME_DATA->specularBuffer;
    checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, m_metallicPSOHandle, m_rs,
               m_constBufferHandle);
    break;
  }
  case (DebugIndex::ROUGHNESS_BUFFER): {
    const TextureHandle input = globals::DEBUG_FRAME_DATA->specularBuffer;
    checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, m_roughnessPSOHandle, m_rs,
               m_constBufferHandle);
    break;
  }
  case (DebugIndex::THICKNESS_BUFFER): {
    const TextureHandle input = globals::DEBUG_FRAME_DATA->specularBuffer;
    checkHandle(input, handleToWriteOn);
    blitBuffer(input, handleToWriteOn, m_thicknessPSOHandle, m_rs,
               m_constBufferHandle);
    break;
  }
  case (DebugIndex::GBUFFER_DEPTH): {
    const TextureHandle input = globals::DEBUG_FRAME_DATA->gbufferDepth;
    checkHandle(input, handleToWriteOn);
    reduceDepth(globals::DEBUG_FRAME_DATA->gbufferDepth);
    blitDepthDebug(input, handleToWriteOn, m_reduceBufferHandle,
                   m_depthPSOHandle, m_rs, m_constBufferHandle,
                   m_reduceBufferHandle);
    break;
  }
  case (DebugIndex::GBUFFER_STENCIL): {
    const TextureHandle input = globals::DEBUG_FRAME_DATA->gbufferDepth;
    blitStencilDebug(input, handleToWriteOn, m_stencilPSOHandle, m_rs,
                     m_config);
    break;
  }
  default:
    // assert(0 && "no valid pass to debug");
    break;
  }
}

void FramePassDebugNode::updateConstantBuffer() {
  globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferBuffered(
      m_constBufferHandle, &m_config);
  m_updateConfig = false;
}

void FramePassDebugNode::reduceDepth(const TextureHandle source) const {
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
  commandList->SetComputeRootSignature(m_reduceRs);

  // binding the resources
  dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(source);
  // bind the source
  commandList->SetComputeRootDescriptorTable(2, pair.gpuHandle);
  // setup the
  // commandList->SetComputeRootDescriptorTable(
  //    2, dx12::CONSTANT_BUFFER_MANAGER
  //           ->getConstantBufferDx12Handle(m_textureConfigHandle)
  //           .gpuHandle);

  dx12::RENDERING_CONTEXT->bindCameraBufferCompute(0);
  dx12::BUFFER_MANAGER->bindBuffer(m_reduceBufferHandle, 1, commandList);
  // first we kick the clear
  dx12::PSO_MANAGER->bindPSO(m_depthReduceClearPSOHandle, commandList);
  commandList->Dispatch(1, 1, 1);

  // we need to issue a barrier here, both shaders write to the reduce buffer
  counter = 0;
  counter = dx12::BUFFER_MANAGER->bufferUAVTransition(m_reduceBufferHandle,
                                                      barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  dx12::PSO_MANAGER->bindPSO(m_depthReducePSOHandle, commandList);

  const uint32_t width = globals::ENGINE_CONFIG->m_windowWidth;
  const uint32_t height = globals::ENGINE_CONFIG->m_windowHeight;
  const uint32_t blockWidth = 64;
  const uint32_t blockHeight = 4;
  const uint32_t gx =
      width / blockWidth == 0 ? width / blockWidth : (width / blockWidth) + 1;
  const uint32_t gy = height / blockHeight == 0 ? height / blockHeight
                                                : height / blockHeight + 1;
  commandList->Dispatch(gx, gy, 1);
}

void FramePassDebugNode::initialize() {
  m_constBufferHandle = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(DebugLayerConfig), &m_config);

  m_reduceBufferHandle = globals::BUFFER_MANAGER->allocate(
      sizeof(ReducedDepth), nullptr, "depthDebugReduce", 1,
      sizeof(ReducedDepth), true);
  m_config.stencilValue = 1;
}

void FramePassDebugNode::compute() {
  // get the render texture
  annotateGraphicsBegin("DebugPass");

  // check if we need to update the constant buffer
  if (m_updateConfig) {
    updateConstantBuffer();
  }

#if SE_DEBUG
  blitDebugFrame(inputRTHandle);
#endif
  annotateGraphicsEnd();
}
} // namespace SirEngine
