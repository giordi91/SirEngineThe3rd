#include "SirEngine/graphics/nodes/deferredLighting.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"

namespace SirEngine {

static const char *DEFERRED_LIGHTING_PSO = "deferredDirectionalLight_PSO";
static const char *DEFERRED_LIGHTING_RS = "deferredDirectionalLight_RS";

DeferredLightingPass::DeferredLightingPass(GraphAllocators &allocators)
    : GNode("DeferredLightingPass", "DeferredLightingPass", allocators) {
  // init data
  defaultInitializePlugsAndConnections(5, 1);

  // lets create the plugs
  GPlug &geometryBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::GEOMETRY_RT)];
  geometryBuffer.plugValue = 0;
  geometryBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  geometryBuffer.nodePtr = this;
  geometryBuffer.name = "geometry";

  GPlug &normalBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::NORMALS_RT)];
  normalBuffer.plugValue = 0;
  normalBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  normalBuffer.nodePtr = this;
  normalBuffer.name = "normal";

  GPlug &specularBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::SPECULAR_RT)];
  specularBuffer.plugValue = 0;
  specularBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  specularBuffer.nodePtr = this;
  specularBuffer.name = "specular";

  GPlug &depthBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";

  GPlug &shadowBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DIRECTIONAL_SHADOW_RT)];
  shadowBuffer.plugValue = 0;
  shadowBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  shadowBuffer.nodePtr = this;
  shadowBuffer.name = "shadow";

  GPlug &lightBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::LIGHTING_RT)];
  lightBuffer.plugValue = 0;
  lightBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  lightBuffer.nodePtr = this;
  lightBuffer.name = "lighting";

  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      DEFERRED_LIGHTING_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(DEFERRED_LIGHTING_PSO);
}

void DeferredLightingPass::initialize() {

  // HDR Buffer
  m_lightBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight,
      RenderTargetFormat::R16G16B16A16_FLOAT, "lightBuffer");

  m_lightCB = dx12::RENDERING_CONTEXT->getLightCB();
  m_lightAddress = dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(m_lightCB);
  m_brdfHandle = dx12::RENDERING_CONTEXT->getBrdfHandle();
}

inline TextureHandle getInputConnection(ResizableVector<const GPlug *> **conns,
                                        int plugId) {
  const auto conn = conns[PLUG_INDEX(plugId)];

  // TODO not super safe to do this, might be worth improving this
  assert(conn->size() == 1 && "too many input connections");
  const GPlug *source = (*conn)[0];
  const auto h = TextureHandle{source->plugValue};
  assert(h.isHandleValid());
  return h;
}

void DeferredLightingPass::compute() {

  annotateGraphicsBegin("DeferredLightingPass");

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  assert(gbufferHandle.isHandleValid());
  assert(normalBufferHandle.isHandleValid());
  assert(specularBufferHandle.isHandleValid());
  assert(depthHandle.isHandleValid());

  // commandList->SetPipelineState(pso);
  dx12::PSO_MANAGER->bindPSO(pso, commandList);

  D3D12_RESOURCE_BARRIER barriers[5];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      depthHandle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers,
      counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      gbufferHandle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers,
      counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      normalBufferHandle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers,
      counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      specularBufferHandle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
      barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_lightBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(m_lightBuffer, TextureHandle{});
  commandList->SetGraphicsRootSignature(rs);

  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  commandList->SetGraphicsRootConstantBufferView(1, m_lightAddress);
  commandList->SetGraphicsRootDescriptorTable(
      2, dx12::TEXTURE_MANAGER->getSRVDx12(depthHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      3, dx12::TEXTURE_MANAGER->getSRVDx12(gbufferHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      4, dx12::TEXTURE_MANAGER->getSRVDx12(normalBufferHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      5, dx12::TEXTURE_MANAGER->getSRVDx12(specularBufferHandle).gpuHandle);
  const TextureHandle skyHandle =
      dx12::RENDERING_CONTEXT->getEnviromentMapIrradianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      6, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

  // TODO: investigate bug of lighting pass, irradiance seems to be
  // weirdly rotated
  const TextureHandle skyRadianceHandle =
      dx12::RENDERING_CONTEXT->getEnviromentMapRadianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      7, dx12::TEXTURE_MANAGER->getSRVDx12(skyRadianceHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      8, dx12::TEXTURE_MANAGER->getSRVDx12(m_brdfHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      9, dx12::TEXTURE_MANAGER
             ->getSRVDx12(globals::DEBUG_FRAME_DATA->directionalShadow)
             .gpuHandle);

  // the newer ID3DUserDefinedAnnotation API is also supported
  currentFc->commandList->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->DrawInstanced(6, 1, 0, 0);
  annotateGraphicsEnd();
}

inline void freeTextureIfValid(TextureHandle h) {
  if (h.isHandleValid()) {
    dx12::TEXTURE_MANAGER->free(h);
    h.handle = 0;
  }
}

void DeferredLightingPass::clear() {
  freeTextureIfValid(m_lightBuffer);
  m_generation = -1;
}

void DeferredLightingPass::onResizeEvent(int, int) {
  clear();
  initialize();
}

void DeferredLightingPass::populateNodePorts() {
  gbufferHandle = getInputConnection(m_inConnections, GEOMETRY_RT);
  normalBufferHandle = getInputConnection(m_inConnections, NORMALS_RT);
  specularBufferHandle = getInputConnection(m_inConnections, SPECULAR_RT);
  depthHandle = getInputConnection(m_inConnections, DEPTH_RT);

  m_outputPlugs[0].plugValue = m_lightBuffer.handle;
}
} // namespace SirEngine
