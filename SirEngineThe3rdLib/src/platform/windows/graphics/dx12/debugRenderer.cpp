#include "platform/windows/graphics/dx12/debugRenderer.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/bufferManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

namespace SirEngine::dx12 {
void DebugRenderer::init() {
  // lests build the map of rs and pso
  // points single color
  PSOHandle psoHandle =
      dx12::PSO_MANAGER->getHandleFromName("debugDrawPointsSingleColorPSO");
  RSHandle rsHandle = dx12::ROOT_SIGNATURE_MANAGER->getHandleFromName(
      "debugDrawPointsSingleColorRS");

  m_shderTypeToShaderBind[static_cast<uint16_t>(
      SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR)] =
      ShaderBind{rsHandle, psoHandle};
}
void DebugRenderer::cleanPerFrame() {
  /*
for (auto &queue : m_renderables) {
for (auto &prim : queue.second) {
// dx12::CONSTANT_BUFFER_MANAGER->free
// prim.cbHandle
dx12::CON
}
}
*/
}

static ID3D12Resource *createDefaultBuffer(ID3D12Device *device,
                                           ID3D12GraphicsCommandList *cmdList,
                                           const void *initData,
                                           const UINT64 byteSize,
                                           ID3D12Resource **uploadBuffer) {
  ID3D12Resource *defaultBuffer = nullptr;

  // Create the actual default buffer resource.
  auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto defaultBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
  HRESULT res = device->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &defaultBufferDesc,
      D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&defaultBuffer));
  assert(SUCCEEDED(res));

  // In order to copy CPU memory data into our default buffer, we need to create
  // an intermediate upload heap.
  auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
  res = device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer));
  assert(SUCCEEDED(res));

  // Describe the data we want to copy into the default buffer.
  D3D12_SUBRESOURCE_DATA subResourceData = {};
  subResourceData.pData = initData;
  subResourceData.RowPitch = byteSize;
  subResourceData.SlicePitch = subResourceData.RowPitch;

  // Schedule to copy the data to the default buffer resource.  At a high level,
  // the helper function UpdateSubresources will copy the CPU memory into the
  // intermediate upload heap.  Then, using
  // ID3D12CommandList::CopySubresourceRegion, the intermediate upload heap data
  // will be copied to mBuffer.
  auto preTransition = CD3DX12_RESOURCE_BARRIER::Transition(
      defaultBuffer, D3D12_RESOURCE_STATE_COMMON,
      D3D12_RESOURCE_STATE_COPY_DEST);
  cmdList->ResourceBarrier(1, &preTransition);
  UpdateSubresources<1>(cmdList, defaultBuffer, *uploadBuffer, 0, 0, 1,
                        &subResourceData);
  auto postTransition = CD3DX12_RESOURCE_BARRIER::Transition(
      defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_GENERIC_READ);
  cmdList->ResourceBarrier(1, &postTransition);

  // Note: uploadBuffer has to be kept alive after the above function calls
  // because the command list has not been executed yet that performs the actual
  // copy. The caller can Release the uploadBuffer after it knows the copy has
  // been executed.
  return defaultBuffer;
}
inline D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(ID3D12Resource *buffer,
                                                    uint32_t strideInByte,
                                                    uint32_t sizeInByte) {

  D3D12_VERTEX_BUFFER_VIEW vbv;
  vbv.BufferLocation = buffer->GetGPUVirtualAddress();
  vbv.StrideInBytes = strideInByte;
  vbv.SizeInBytes = sizeInByte;
  return vbv;
}

DebugDrawHandle DebugRenderer::drawPoints(float *data, uint32_t sizeInByte,
                                          DirectX::XMFLOAT4 color, float size,
                                          bool isPeristen,
                                          const char *debugName) {
  BufferUploadResource upload;
  DebugPrimitive primitive;
  assert(isPeristen);
  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 3)) == 0);
  uint32_t elementCount = sizeInByte / (sizeof(float) * 3);

  primitive.buffer = createDefaultBuffer(
      dx12::DEVICE, dx12::CURRENT_FRAME_RESOURCE->fc.commandList, data,
      sizeInByte, &upload.uploadBuffer);
  // set a signal for the resource.
  upload.fence = dx12::insertFenceToGlobalQueue();
  m_uploadRequests.push_back(upload);
  primitive.bufferView =
      getVertexBufferView(primitive.buffer, sizeof(float) * 3, sizeInByte);

  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(primitive.srv,primitive.buffer,elementCount,sizeof(float)*3);

  // allocate constant buffer
  DebugPointsFixedColor settings{color, size};
  ConstantBufferHandle chandle = dx12::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(settings), &settings);

  // store it such way that we can render it
  primitive.primitiveType = PrimitiveType::POINT;
  primitive.cbHandle = chandle;
  primitive.primitiveToRender = elementCount * 6;

  DebugDrawHandle debugHandle{++MAGIC_NUMBER_COUNTER};

  // generate handle for storing
  SHADER_QUEUE_FLAGS queue = SHADER_QUEUE_FLAGS::DEBUG;
  SHADER_TYPE_FLAGS type = SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR;
  uint32_t storeHandle =
      static_cast<uint32_t>(queue) | (static_cast<uint32_t>(type) << 16);

  if (isPeristen) {
    m_renderablesPersistant[storeHandle].push_back(primitive);
  } else {
    m_renderablesPersistant[storeHandle].push_back(primitive);
  }
  return debugHandle;
}
void DebugRenderer::render() {

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  // first static stuff
  for (auto &queue : m_renderablesPersistant) {
    assert((dx12::MATERIAL_MANAGER->isQueueType(queue.first,
                                                SHADER_QUEUE_FLAGS::DEBUG)));
    constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
    const auto typeFlags = static_cast<uint16_t>((queue.first & mask) >> 16);

    const auto found = m_shderTypeToShaderBind.find(typeFlags);
    if (found != m_shderTypeToShaderBind.end()) {
      dx12::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(found->second.rs,
                                                   commandList);
      dx12::PSO_MANAGER->bindPSO(found->second.pso, commandList);
    } else {
      assert(!"Could not find debug pso or rs");
    }

    // this is most for debug, it will boil down to nothing in release
    const SHADER_TYPE_FLAGS type =
        dx12::MATERIAL_MANAGER->getTypeFlags(queue.first);
    const std::string &typeName =
        dx12::MATERIAL_MANAGER->getStringFromShaderTypeFlag(type);
    annotateGraphicsBegin(typeName.c_str());
    globals::RENDERING_CONTEXT->bindCameraBuffer(0);

    for (auto &prim : queue.second) {

      int x = 0;
      // const auto address =
      //    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(prim.cbHandle);
      // commandList->SetGraphicsRootConstantBufferView(1, address);

      commandList->SetGraphicsRootDescriptorTable(
          1, dx12::CONSTANT_BUFFER_MANAGER
                 ->getConstantBufferDx12Handle(prim.cbHandle)
                 .gpuHandle);

      //currentFc->commandList->IASetVertexBuffers(0, 1, &prim.bufferView);
	  //currentFc->commandList->SetGraphicsRootShaderResourceView(1,prim.buffer->GetGPUVirtualAddress());
      commandList->SetGraphicsRootDescriptorTable(2,prim.srv.gpuHandle);

      currentFc->commandList->IASetVertexBuffers(0, 1, nullptr);
      currentFc->commandList->IASetPrimitiveTopology(
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      currentFc->commandList->DrawInstanced(prim.primitiveToRender, 1, 0, 0);
    }

    annotateGraphicsEnd();
  }
} // namespace SirEngine::dx12
void DebugRenderer::clearUploadRequests() {

  auto id = GLOBAL_FENCE->GetCompletedValue();
  // uint32_t freed = 0;
  int requestSize = static_cast<int>(m_uploadRequests.size()) - 1;
  int stackTopIdx = requestSize;
  for (int i = requestSize; i >= 0; --i) {
    BufferUploadResource &upload = m_uploadRequests[i];
    if (upload.fence < id) {
      // we can free the memory
      upload.uploadBuffer->Release();
      if (stackTopIdx != i) {
        // lets copy
        m_uploadRequests[i] = m_uploadRequests[stackTopIdx];
      }
      --stackTopIdx;
    }
  }
  // resizing the vector
  m_uploadRequests.resize(stackTopIdx + 1);
}
} // namespace SirEngine::dx12