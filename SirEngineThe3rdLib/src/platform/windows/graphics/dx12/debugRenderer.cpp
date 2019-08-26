#include "platform/windows/graphics/dx12/debugRenderer.h"
#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/stringPool.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
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

  psoHandle =
      dx12::PSO_MANAGER->getHandleFromName("debugDrawLinesSingleColorPSO");
  rsHandle = dx12::ROOT_SIGNATURE_MANAGER->getHandleFromName(
      "debugDrawLinesSingleColorRS");

  m_shderTypeToShaderBind[static_cast<uint16_t>(
      SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR)] =
      ShaderBind{rsHandle, psoHandle};
}
void DebugRenderer::cleanPerFrame() {

  /*
for (auto &prim : m_primToFree) {
prim.buffer->Release();
dx12::CONSTANT_BUFFER_MANAGER->free(prim.cbHandle);
}
*/
  // for (auto &queue : m_renderables) {
  //  for (auto &prim : queue.second) {
  //    // dx12::CONSTANT_BUFFER_MANAGER->free
  //  }
  //}
}

inline ID3D12Resource *
allocateUploadBuffer(ID3D12Device *pDevice, void *pData,
                     const uint64_t dataSize, void **pMappedData,
                     const wchar_t *resourceName = nullptr) {
  ID3D12Resource *ppResource;
  auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
  HRESULT res = pDevice->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ppResource));

  if (FAILED(res)) {
    SE_CORE_ERROR("error allocating uppload buffer");
  }
  if (resourceName) {
    (ppResource)->SetName(resourceName);
  }
  (ppResource)->Map(0, nullptr, pMappedData);
  memcpy(*pMappedData, pData, dataSize);
  //(ppResource)->Unmap(0, nullptr);
  return ppResource;
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

DebugDrawHandle DebugRenderer::drawPointsUniformColor(float *data,
                                                      uint32_t sizeInByte,
                                                      DirectX::XMFLOAT4 color,
                                                      float size,
                                                      const char *debugName) {
  BufferUploadResource upload;
  DebugPrimitive primitive;
  void *mappedData;
  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 3)) == 0);
  const uint32_t elementCount = sizeInByte / (sizeof(float) * 3);

  // primitive.buffer = createDefaultBuffer(
  //    dx12::DEVICE, dx12::CURRENT_FRAME_RESOURCE->fc.commandList, data,
  //    sizeInByte, &upload.uploadBuffer);
  primitive.buffer = allocateUploadBuffer(
      dx12::DEVICE, data, sizeInByte, &mappedData, frameConvertWide(debugName));

  /*
  // set a signal for the resource.
  upload.fence = dx12::insertFenceToGlobalQueue();
  m_uploadRequests.push_back(upload);
  */
  primitive.bufferView =
      getVertexBufferView(primitive.buffer, sizeof(float) * 3, sizeInByte);

  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
      primitive.srv, primitive.buffer, elementCount, sizeof(float) * 3);

  // allocate constant buffer
  DebugPointsFixedColor settings{color, size};
  const ConstantBufferHandle chandle =
      dx12::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(settings),
                                                     &settings);

  // store it such way that we can render it
  primitive.primitiveType = PRIMITIVE_TYPE::POINT;
  primitive.cbHandle = chandle;
  primitive.primitiveToRender = elementCount * 6;

  // generate handle for storing
  SHADER_QUEUE_FLAGS queue = SHADER_QUEUE_FLAGS::DEBUG;
  SHADER_TYPE_FLAGS type = SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR;
  const uint32_t storeHandle =
      static_cast<uint32_t>(queue) | (static_cast<uint32_t>(type) << 16);

  DebugTracker tracker{};
  tracker.compoundCount = 0;
  tracker.index = m_renderables[storeHandle].size();
  tracker.magicNumber = MAGIC_NUMBER_COUNTER;
  tracker.queue = storeHandle;
  tracker.mappedData = mappedData;
  tracker.sizeInBtye = sizeInByte;

  const DebugDrawHandle debugHandle{(MAGIC_NUMBER_COUNTER << 16) |
                                    tracker.index};

  // registering the tracker
  m_trackers[debugHandle.handle] = tracker;
  // registering the renderables
  m_renderables[storeHandle].push_back(primitive);

  ++MAGIC_NUMBER_COUNTER;

  return debugHandle;
}

DebugDrawHandle
DebugRenderer::drawLinesUniformColor(float *data, const uint32_t sizeInByte,
                                     const DirectX::XMFLOAT4 color,
                                     const float size, const char *debugName) {
  DebugPrimitive primitive;

  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 3)) == 0);
  const uint32_t elementCount = sizeInByte / (sizeof(float) * 3);

  /*
  BufferUploadResource upload;
  primitive.buffer = createDefaultBuffer(
      dx12::DEVICE, dx12::CURRENT_FRAME_RESOURCE->fc.commandList, data,
      sizeInByte, &upload.uploadBuffer);
          */
  void *mappedData;
  primitive.buffer = allocateUploadBuffer(
      dx12::DEVICE, data, sizeInByte, &mappedData, frameConvertWide(debugName));
  /*
  // set a signal for the resource.
  upload.fence = dx12::insertFenceToGlobalQueue();
  m_uploadRequests.push_back(upload);
  */

  primitive.bufferView =
      getVertexBufferView(primitive.buffer, sizeof(float) * 3, sizeInByte);

  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
      primitive.srv, primitive.buffer, elementCount, sizeof(float) * 3);

  // allocate constant buffer
  DebugPointsFixedColor settings{color, size};
  const ConstantBufferHandle chandle =
      dx12::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(settings),
                                                     &settings);

  // store it such way that we can render it
  primitive.primitiveType = PRIMITIVE_TYPE::LINE;
  primitive.cbHandle = chandle;
  primitive.primitiveToRender = static_cast<int>(elementCount);

  // generate handle for storing
  SHADER_QUEUE_FLAGS queue = SHADER_QUEUE_FLAGS::DEBUG;
  SHADER_TYPE_FLAGS type = SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR;
  const uint32_t storeHandle =
      static_cast<uint32_t>(queue) | (static_cast<uint32_t>(type) << 16);

  DebugTracker tracker;
  tracker.compoundCount = 0;
  tracker.index = m_renderables[storeHandle].size();
  tracker.magicNumber = MAGIC_NUMBER_COUNTER;
  tracker.queue = storeHandle;
  tracker.mappedData = mappedData;
  tracker.sizeInBtye = sizeInByte;

  const DebugDrawHandle debugHandle{(MAGIC_NUMBER_COUNTER << 16) |
                                    tracker.index};

  // registering the tracker
  m_trackers[debugHandle.handle] = tracker;
  // registering the renderables
  m_renderables[storeHandle].push_back(primitive);

  ++MAGIC_NUMBER_COUNTER;

  return debugHandle;
}

DebugDrawHandle DebugRenderer::drawSkeleton(Skeleton *skeleton,
                                            DirectX::XMFLOAT4 color,
                                            float pointSize) {

  // TODO cleanup all this lovely vectors

  // first we need to convert the skeleton to points we can actually render
  std::vector<DirectX::XMFLOAT3> points;
  std::vector<DirectX::XMFLOAT3> lines;
  const std::vector<Joint> &joints = skeleton->m_joints;
  for (int i = 0; i < joints.size(); ++i) {
    DirectX::XMMATRIX inv = joints[i].m_inv_bind_pose;
    DirectX::XMMATRIX mat = DirectX::XMMatrixInverse(nullptr, inv);
    DirectX::XMVECTOR pos;
    DirectX::XMVECTOR scale;
    DirectX::XMVECTOR rot;
    DirectX::XMMatrixDecompose(&scale, &rot, &pos, mat);
    points.push_back(
        DirectX::XMFLOAT3{pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]});

    if (joints[i].m_parent_id != -1) {
      lines.push_back(points[joints[i].m_parent_id]);
      lines.push_back(
          DirectX::XMFLOAT3{pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]});
    }
  }
  DebugDrawHandle pointsHandle = drawPointsUniformColor(
      &points.data()[0].x, points.size() * sizeof(DirectX::XMFLOAT3), color,
      pointSize, skeleton->m_name.c_str());

  DebugDrawHandle linesHandle = drawLinesUniformColor(
      &lines.data()[0].x, lines.size() * sizeof(DirectX::XMFLOAT3), color,
      pointSize, skeleton->m_name.c_str());

  // lets prepare the compound handle
  // there are two items only lines and points and the points is the first
  DebugTracker tracker{};
  tracker.compoundCount = 2;
  // TODO remove this naked allocation use an allocator
  // being compound we only store the compound count and handles
  tracker.compoundHandles = new DebugDrawHandle[2];
  tracker.compoundHandles[0] = pointsHandle;
  tracker.compoundHandles[1] = linesHandle;

  uint32_t compoundBit = 1 << 31;
  const DebugDrawHandle returnHandle{compoundBit |
                                     (MAGIC_NUMBER_COUNTER << 16) | 0};

  m_trackers[returnHandle.handle] = tracker;

  ++MAGIC_NUMBER_COUNTER;
  return returnHandle;
}

DebugDrawHandle DebugRenderer::drawAnimatedSkeleton(DebugDrawHandle handle,
                                                    AnimState *state,
                                                    DirectX::XMFLOAT4 color,
                                                    float pointSize) {

  const std::vector<DirectX::XMMATRIX> &pose = state->m_pose->m_worldMat;
  std::vector<DirectX::XMFLOAT3> points;
  std::vector<DirectX::XMFLOAT3> lines;
  for (int i = 0; i < pose.size(); ++i) {
    DirectX::XMMATRIX mat = pose[i];
    DirectX::XMVECTOR pos;
    DirectX::XMVECTOR scale;
    DirectX::XMVECTOR rot;
    DirectX::XMMatrixDecompose(&scale, &rot, &pos, mat);
    points.push_back(
        DirectX::XMFLOAT3{pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]});

    int parentId = state->m_pose->m_skeleton->m_joints[i].m_parent_id;
    if (parentId != -1) {
      lines.push_back(points[parentId]);
      lines.push_back(
          DirectX::XMFLOAT3{pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]});
    }
  }

  // if handle is null means we need to allocate, if not we reuse the memory
  if (handle.isHandleValid()) {
    // we need to allocate the memory
    // making sure our handle is an actual compound handle
    assert((handle.handle & (1 << 31)) > 0);
    // extact from the tracker map
    auto found = m_trackers.find(handle.handle);
    assert(found != m_trackers.end());
    assert(found->second.compoundCount == 2);

    // we have the tracker so we should be able to get out data
    const DebugDrawHandle pointsHandle = found->second.compoundHandles[0];
    const DebugDrawHandle linesHandle = found->second.compoundHandles[1];

    // lets get the trackers out for each one
    auto foundPoint = m_trackers.find(pointsHandle.handle);
    assert(foundPoint != m_trackers.end());
    assert(foundPoint->second.compoundCount == 0);

    const DebugTracker &pointTracker = foundPoint->second;
    assert(pointTracker.sizeInBtye ==
           (sizeof(DirectX::XMFLOAT3) * points.size()));
    memcpy(pointTracker.mappedData, points.data(), pointTracker.sizeInBtye);

    auto foundLines= m_trackers.find(linesHandle.handle);
    assert(foundLines != m_trackers.end());
    assert(foundLines->second.compoundCount == 0);

    const DebugTracker &lineTracker = foundLines->second;
    assert(lineTracker.sizeInBtye ==
           (sizeof(DirectX::XMFLOAT3) * lines.size()));
    memcpy(lineTracker.mappedData, lines.data(), lineTracker.sizeInBtye);

	//data is updated we are good to go, returning same handle
	//since a new one has not been allocated
	return handle;


  } else {
    DebugDrawHandle pointsHandle = drawPointsUniformColor(
        &points.data()[0].x, points.size() * sizeof(DirectX::XMFLOAT3), color,
        pointSize, state->m_pose->m_skeleton->m_name.c_str());

    DebugDrawHandle linesHandle = drawLinesUniformColor(
        &lines.data()[0].x, lines.size() * sizeof(DirectX::XMFLOAT3), color,
        pointSize, state->m_pose->m_skeleton->m_name.c_str());

    // lets prepare the compound handle
    // there are two items only lines and points and the points is the first
    DebugTracker tracker{};
    tracker.compoundCount = 2;
    // TODO remove this naked allocation use an allocator
    // being compound we only store the compound count and handles
    tracker.compoundHandles = new DebugDrawHandle[2];
    tracker.compoundHandles[0] = pointsHandle;
    tracker.compoundHandles[1] = linesHandle;

    uint32_t compoundBit = 1 << 31;
    const DebugDrawHandle returnHandle{compoundBit |
                                       (MAGIC_NUMBER_COUNTER << 16) | 0};

    m_trackers[returnHandle.handle] = tracker;

    ++MAGIC_NUMBER_COUNTER;

    return returnHandle;
  }
}

void DebugRenderer::renderQueue(
    std::unordered_map<uint32_t, std::vector<DebugPrimitive>> &inQueue,
    const TextureHandle input, const TextureHandle depth) {

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  for (auto &queue : inQueue) {
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

    int counter = 0;
    D3D12_RESOURCE_BARRIER barriers[2];
    counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
        depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);
    counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
        input, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

    if (counter) {
      commandList->ResourceBarrier(counter, barriers);
    }

    globals::TEXTURE_MANAGER->bindRenderTarget(input, depth);
    for (auto &prim : queue.second) {

      int x = 0;
      // const auto address =
      //    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(prim.cbHandle);
      // commandList->SetGraphicsRootConstantBufferView(1, address);

      commandList->SetGraphicsRootDescriptorTable(
          1, dx12::CONSTANT_BUFFER_MANAGER
                 ->getConstantBufferDx12Handle(prim.cbHandle)
                 .gpuHandle);

      // currentFc->commandList->IASetVertexBuffers(0, 1, &prim.bufferView);
      // currentFc->commandList->SetGraphicsRootShaderResourceView(1,prim.buffer->GetGPUVirtualAddress());
      const uint32_t isPC = type == SHADER_TYPE_FLAGS::DEBUG_POINTS_COLORS;
      const uint32_t isPSC =
          type == SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR;
      if (isPC | isPSC) {
        commandList->SetGraphicsRootDescriptorTable(2, prim.srv.gpuHandle);
        currentFc->commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      } else {
        currentFc->commandList->IASetVertexBuffers(0, 1, &prim.bufferView);
        currentFc->commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_LINELIST);
      }

      currentFc->commandList->IASetVertexBuffers(0, 1, nullptr);
      currentFc->commandList->DrawInstanced(prim.primitiveToRender, 1, 0, 0);
    }
  }
}

void DebugRenderer::render(const TextureHandle input,
                           const TextureHandle depth) {

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  // first static stuff
  renderQueue(m_renderables, input, depth);

  // we need to clean up the per frame data

  annotateGraphicsEnd();

  currentFc->commandList->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
} // namespace SirEngine::dx12
void DebugRenderer::clearUploadRequests() {

  auto id = GLOBAL_FENCE->GetCompletedValue();

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