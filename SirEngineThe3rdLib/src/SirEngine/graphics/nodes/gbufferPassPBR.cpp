#include "SirEngine/graphics/nodes/gbufferPassPBR.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {
static const char *GBUFFER_RS = "gbufferPBRRS";
static const char *GBUFFER_PSO = "gbufferPBRPSO";

GBufferPassPBR::GBufferPassPBR(const char *name)
    : GraphNode(name, "GBufferPassPBR") {
  // lets create the plugs
  Plug geometryBuffer;
  geometryBuffer.plugValue = 0;
  geometryBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  geometryBuffer.nodePtr = this;
  geometryBuffer.name = "geometry";
  registerPlug(geometryBuffer);

  Plug normalBuffer;
  normalBuffer.plugValue = 0;
  normalBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  normalBuffer.nodePtr = this;
  normalBuffer.name = "normal";
  registerPlug(normalBuffer);

  Plug specularBuffer;
  specularBuffer.plugValue = 0;
  specularBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  specularBuffer.nodePtr = this;
  specularBuffer.name = "specular";
  registerPlug(specularBuffer);

  Plug depthBuffer;
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";
  registerPlug(depthBuffer);

  // lets create the plugs
  Plug stream;
  stream.plugValue = 0;
  stream.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
  stream.nodePtr = this;
  stream.name = "assetStream";
  registerPlug(stream);

  // fetching root signature
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(GBUFFER_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(GBUFFER_PSO);
}

void GBufferPassPBR::initialize() {

  m_depth = dx12::TEXTURE_MANAGER->createDepthTexture(
      "gbufferDepth", globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT);

  m_geometryBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT, RenderTargetFormat::RGBA32,
      "geometryBuffer");

  m_normalBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT,
      RenderTargetFormat::R11G11B10_UNORM, "normalBuffer");

  m_specularBuffer = globals::TEXTURE_MANAGER->allocateRenderTexture(
      globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT, RenderTargetFormat::RGBA32,
      "specularBuffer");
}

void GBufferPassPBR::compute() {

  annotateGraphicsBegin("GBufferPassPBR");

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  dx12::PSO_MANAGER->bindPSO(pso, commandList);

  D3D12_RESOURCE_BARRIER barriers[4];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_geometryBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_normalBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_specularBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->clearDepth(m_depth, 0.0f);
  float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  globals::TEXTURE_MANAGER->clearRT(m_geometryBuffer, color);
  globals::TEXTURE_MANAGER->clearRT(m_normalBuffer, color);
  globals::TEXTURE_MANAGER->clearRT(m_specularBuffer, color);

  D3D12_CPU_DESCRIPTOR_HANDLE handles[3] = {
      dx12::TEXTURE_MANAGER->getRTVDx12(m_geometryBuffer).cpuHandle,
      dx12::TEXTURE_MANAGER->getRTVDx12(m_normalBuffer).cpuHandle,
      dx12::TEXTURE_MANAGER->getRTVDx12(m_specularBuffer).cpuHandle};

  auto depthDescriptor = dx12::TEXTURE_MANAGER->getRTVDx12(m_depth).cpuHandle;
  commandList->SetGraphicsRootSignature(rs);
  commandList->OMSetRenderTargets(3, handles, false, &depthDescriptor);

  globals::RENDERING_CONTEX->bindCameraBuffer(0);

  const std::unordered_map<uint32_t, std::vector<Renderable>> &renderables =
      globals::ASSET_MANAGER->getRenderables(
          StreamHandle{m_inputPlugs[0].plugValue});

  for (const auto &renderableList : renderables) {
    if (dx12::MATERIAL_MANAGER->isQueueType(renderableList.first,
                                            SHADER_QUEUE_FLAGS::DEFERRED)) {
      const SHADER_TYPE_FLAGS type =
          dx12::MATERIAL_MANAGER->getTypeFlags(renderableList.first);
      const std::string &typeName =
          dx12::MATERIAL_MANAGER->getStringFromShaderTypeFlag(type);
      annotateGraphicsBegin(typeName.c_str());

      const size_t count = renderableList.second.size();
      const Renderable *currRenderables = renderableList.second.data();
      for (int i = 0; i < count; ++i) {
        const Renderable &renderable = currRenderables[i];
        commandList->SetGraphicsRootConstantBufferView(
            1, renderable.m_materialRuntime.cbVirtualAddress);
        commandList->SetGraphicsRootDescriptorTable(
            2, renderable.m_materialRuntime.albedo);
        commandList->SetGraphicsRootDescriptorTable(
            3, renderable.m_materialRuntime.normal);
        commandList->SetGraphicsRootDescriptorTable(
            4, renderable.m_materialRuntime.metallic);
        commandList->SetGraphicsRootDescriptorTable(
            5, renderable.m_materialRuntime.roughness);
        dx12::MESH_MANAGER->bindMeshRuntimeAndRender(renderable.m_meshRuntime,
                                                     currentFc);
      }
      annotateGraphicsEnd();
    }
  }

  m_outputPlugs[0].plugValue = m_geometryBuffer.handle;
  m_outputPlugs[1].plugValue = m_normalBuffer.handle;
  m_outputPlugs[2].plugValue = m_specularBuffer.handle;
  m_outputPlugs[3].plugValue = m_depth.handle;

#if SE_DEBUG
  globals::DEBUG_FRAME_DATA->geometryBuffer = m_geometryBuffer;
  globals::DEBUG_FRAME_DATA->normalBuffer = m_normalBuffer;
  globals::DEBUG_FRAME_DATA->specularBuffer = m_specularBuffer;
  globals::DEBUG_FRAME_DATA->gbufferDepth = m_depth;
#endif
  annotateGraphicsEnd();
}

#define FREE_TEXTURE_IF_VALID(h)                                               \
  if ((h).isHandleValid()) {                                                   \
    dx12::TEXTURE_MANAGER->free(h);                                            \
    (h).handle = 0;                                                            \
  }

void GBufferPassPBR::clear() {
  FREE_TEXTURE_IF_VALID(m_depth)
  FREE_TEXTURE_IF_VALID(m_geometryBuffer)
  FREE_TEXTURE_IF_VALID(m_normalBuffer)
  FREE_TEXTURE_IF_VALID(m_specularBuffer)
}

void GBufferPassPBR::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
