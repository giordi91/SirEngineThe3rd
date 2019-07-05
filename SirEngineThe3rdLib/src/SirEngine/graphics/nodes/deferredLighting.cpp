#include "SirEngine/graphics/nodes/deferredLighting.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {

static const char *DEFERRED_LIGHTING_PSO = "deferredDirectionalLight_PSO";
static const char *DEFERRED_LIGHTING_RS = "deferredDirectionalLight_RS";

DeferredLightingPass::DeferredLightingPass(const char *name)
    : GraphNode(name, "DeferredLightingPass") {
  // lets create the plugs
  Plug geometryBuffer;
  geometryBuffer.plugValue = 0;
  geometryBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  geometryBuffer.nodePtr = this;
  geometryBuffer.name = "geometry";
  registerPlug(geometryBuffer);

  Plug normalBuffer;
  normalBuffer.plugValue = 0;
  normalBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  normalBuffer.nodePtr = this;
  normalBuffer.name = "normal";
  registerPlug(normalBuffer);

  Plug specularBuffer;
  specularBuffer.plugValue = 0;
  specularBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  specularBuffer.nodePtr = this;
  specularBuffer.name = "specular";
  registerPlug(specularBuffer);

  Plug depthBuffer;
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";
  registerPlug(depthBuffer);

  Plug lightBuffer;
  lightBuffer.plugValue = 0;
  lightBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  lightBuffer.nodePtr = this;
  lightBuffer.name = "lighting";
  registerPlug(lightBuffer);

  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      DEFERRED_LIGHTING_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(DEFERRED_LIGHTING_PSO);
}

void DeferredLightingPass::initialize() {

  // HDR Buffer
  m_lightBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT,
      RenderTargetFormat::R16G16B16A16_FLOAT, "lightBuffer");

  float intensity = 4.0f;
  m_light.lightColor = {intensity, intensity, intensity, 1.0f};
  m_light.lightDir = {-1.0f, -0.6f, -1.0f, 1.0f};
  m_light.lightPosition = {10.0f, 10.0f, 10.0f, 10.0f};

  // allocate the constant buffer
  m_lightCB = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(DirectionalLightData), &m_light);
  m_lightAddress = dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(m_lightCB);

  m_brdfHandle = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/brdf.texture");
}

inline TextureHandle
getInputConnection(std::unordered_map<const Plug *, std::vector<Plug *>> &conns,
                   Plug *plug) {
  // TODO not super safe to do this, might be worth improving this
  auto &inConns = conns[plug];
  assert(inConns.size() == 1 && "too many input connections");
  Plug *source = inConns[0];
  auto h = TextureHandle{source->plugValue};
  assert(h.isHandleValid());
  return h;
}

void DeferredLightingPass::compute() {

  annotateGraphicsBegin("DeferredLightingPass");

  TextureHandle gbufferHandle =
      getInputConnection(m_connections, &m_inputPlugs[0]);
  TextureHandle normalBufferHandle =
      getInputConnection(m_connections, &m_inputPlugs[1]);
  TextureHandle specularBufferHandle =
      getInputConnection(m_connections, &m_inputPlugs[2]);
  TextureHandle depthHandle =
      getInputConnection(m_connections, &m_inputPlugs[3]);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

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

  globals::RENDERING_CONTEX->bindCameraBuffer(0);
  commandList->SetGraphicsRootConstantBufferView(1, m_lightAddress);
  commandList->SetGraphicsRootDescriptorTable(
      2, dx12::TEXTURE_MANAGER->getSRVDx12(depthHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      3, dx12::TEXTURE_MANAGER->getSRVDx12(gbufferHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      4, dx12::TEXTURE_MANAGER->getSRVDx12(normalBufferHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      5, dx12::TEXTURE_MANAGER->getSRVDx12(specularBufferHandle).gpuHandle);
  TextureHandle skyHandle =
      globals::RENDERING_CONTEX->getEnviromentMapIrradianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      6, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

  TextureHandle skyRadianceHandle =
      globals::RENDERING_CONTEX->getEnviromentMapRadianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      7, dx12::TEXTURE_MANAGER->getSRVDx12(skyRadianceHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      8, dx12::TEXTURE_MANAGER->getSRVDx12(m_brdfHandle).gpuHandle);

  // the newer ID3DUserDefinedAnnotation API is also supported
  commandList->DrawInstanced(6, 1, 0, 0);
  m_outputPlugs[0].plugValue = m_lightBuffer.handle;
  annotateGraphicsEnd();
}

#define FREE_TEXTURE_IF_VALID(h)                                               \
  if (h.isHandleValid()) {                                                     \
    dx12::TEXTURE_MANAGER->free(h);                                            \
    h.handle = 0;                                                              \
  }

void DeferredLightingPass::clear() { FREE_TEXTURE_IF_VALID(m_lightBuffer) }

void DeferredLightingPass::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
