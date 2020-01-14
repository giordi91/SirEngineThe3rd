#include "platform/windows/graphics/dx12/dx12DebugRenderer.h"

#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/animationPlayer.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/memory/stringPool.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12MaterialManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"

namespace SirEngine::dx12 {
void Dx12DebugRenderer::initialize() {
  // lets build the map of rs and pso
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

void Dx12DebugRenderer::free(DebugDrawHandle handle) { assert(0); }

inline ID3D12Resource *allocateUploadBuffer(
    ID3D12Device *pDevice, void *pData, const uint64_t dataSize,
    void **pMappedData, const wchar_t *resourceName = nullptr) {
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

DebugDrawHandle Dx12DebugRenderer::drawPointsUniformColor(
    float *data, const uint32_t sizeInByte, const glm::vec4 color,
    const float size, const char *debugName) {
  // TODO : next time we draw points, we need to fix the code to go from vec3 to
  // vec4 and to bind mesh separately, as buffer and read manually, not input
  // assembler a anymore
  assert(0);
  BufferUploadResource upload;
  DebugPrimitive primitive;
  void *mappedData;
  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 3)) == 0);
  const uint32_t elementCount = sizeInByte / (sizeof(float) * 3);

  // TODO fix this, should use normal buffer manager
  primitive.buffer = allocateUploadBuffer(
      dx12::DEVICE, data, sizeInByte, &mappedData, frameConvertWide(debugName));

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

DebugDrawHandle Dx12DebugRenderer::drawLinesUniformColor(
    float *data, const uint32_t sizeInByte, const glm::vec4 color,
    const float size, const char *debugName) {
  DebugPrimitive primitive;

  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 3)) == 0);
  const uint32_t elementCount = sizeInByte / (sizeof(float) * 3);

  void *mappedData;
  primitive.buffer = allocateUploadBuffer(
      dx12::DEVICE, data, sizeInByte, &mappedData, frameConvertWide(debugName));

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

DebugDrawHandle Dx12DebugRenderer::drawSkeleton(Skeleton *skeleton,
                                                const glm::vec4 color,
                                                const float pointSize) {
  const ResizableVector<glm::mat4> &joints = skeleton->m_jointsWolrdInv;
  // first we need to convert the skeleton to points we can actually render
  auto *points = reinterpret_cast<glm::vec3 *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec3) * joints.size()));
  auto *lines =
      reinterpret_cast<glm::vec3 *>(globals::FRAME_ALLOCATOR->allocate(
          sizeof(glm::vec3) * joints.size() * 2));

  const ResizableVector<int> &parentIds = skeleton->m_parentIds;
  uint32_t lineCounter = 0;
  for (uint32_t i = 0; i < joints.size(); ++i) {
    const glm::mat4 &inv = joints[i];
    const glm::mat4 mat = glm::inverse(inv);

    glm::vec4 pos = mat[3];
    // DirectX::XMVECTOR scale;
    // DirectX::XMVECTOR rot;
    // DirectX::XMMatrixDecompose(&scale, &rot, &pos, mat);
    points[i] = glm::vec3(pos);

    if (parentIds[i] != -1) {
      // here we add a line from the parent to the children, might do a more
      // elaborate joint drawing one day
      lines[lineCounter] = points[parentIds[i]];
      lines[lineCounter + 1] = glm::vec3(pos);
      lineCounter += 2;
    }
  }
  const DebugDrawHandle pointsHandle =
      drawPointsUniformColor(&points[0].x, joints.size() * sizeof(glm::vec3),
                             color, pointSize, skeleton->m_name);

  const DebugDrawHandle linesHandle =
      drawLinesUniformColor(&lines[0].x, lineCounter * sizeof(glm::vec3), color,
                            pointSize, skeleton->m_name);

  // lets prepare the compound handle
  // there are two items only lines and points and the points is the first
  DebugTracker tracker{};
  tracker.compoundCount = 2;
  // TODO remove this naked allocation use an allocator
  // being compound we only store the compound count and handles
  tracker.compoundHandles = new DebugDrawHandle[2];
  tracker.compoundHandles[0] = pointsHandle;
  tracker.compoundHandles[1] = linesHandle;

  const uint32_t compoundBit = 1u << 31u;
  const DebugDrawHandle returnHandle{compoundBit |
                                     (MAGIC_NUMBER_COUNTER << 16) | 0};

  m_trackers[returnHandle.handle] = tracker;

  ++MAGIC_NUMBER_COUNTER;
  return returnHandle;
}

DebugDrawHandle Dx12DebugRenderer::drawAnimatedSkeleton(DebugDrawHandle handle,
                                                        AnimationPlayer *state,
                                                        const glm::vec4 color,
                                                        float pointSize) {
  const glm::mat4 *pose = state->getOutPose()->m_worldMat;
  const uint32_t jointCount = state->getOutPose()->m_skeleton->m_jointCount;

  auto *points = reinterpret_cast<glm::vec3 *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec3) * jointCount));
  auto *lines = reinterpret_cast<glm::vec3 *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec3) * jointCount * 2));

  uint32_t lineCounter = 0;

  for (uint32_t i = 0; i < jointCount; ++i) {
    const glm::mat4 mat = pose[i];
    glm::vec4 pos = mat[3];
    points[i] = glm::vec3(pos);

    const int parentId = state->getOutPose()->m_skeleton->m_parentIds[i];
    if (parentId != -1) {
      // here we add a line from the parent to the children, might do a more
      // elaborate joint drawing one day
      lines[lineCounter] = points[parentId];
      lines[lineCounter + 1] = glm::vec3(pos);
      lineCounter += 2;
    }
  }

  // if handle is null means we need to allocate, if not we reuse the memory
  if (handle.isHandleValid()) {
    // we need to allocate the memory
    // making sure our handle is an actual compound handle
    assert((handle.handle & (1 << 31)) > 0);
    // extact from the tracker map
    const auto found = m_trackers.find(handle.handle);
    assert(found != m_trackers.end());
    assert(found->second.compoundCount == 2);

    // we have the tracker so we should be able to get out data
    const DebugDrawHandle pointsHandle = found->second.compoundHandles[0];
    const DebugDrawHandle linesHandle = found->second.compoundHandles[1];

    // lets get the trackers out for each one
    const auto foundPoint = m_trackers.find(pointsHandle.handle);
    assert(foundPoint != m_trackers.end());
    assert(foundPoint->second.compoundCount == 0);

    const DebugTracker &pointTracker = foundPoint->second;
    assert(pointTracker.sizeInBtye == (sizeof(glm::vec3) * jointCount));
    memcpy(pointTracker.mappedData, points, pointTracker.sizeInBtye);

    const auto foundLines = m_trackers.find(linesHandle.handle);
    assert(foundLines != m_trackers.end());
    assert(foundLines->second.compoundCount == 0);

    const DebugTracker &lineTracker = foundLines->second;
    assert(lineTracker.sizeInBtye == (sizeof(glm::vec3) * lineCounter));
    memcpy(lineTracker.mappedData, lines, lineTracker.sizeInBtye);

    // data is updated we are good to go, returning same handle
    // since a new one has not been allocated
    return handle;

  } else {
    DebugDrawHandle pointsHandle = drawPointsUniformColor(
        &points[0].x, jointCount * sizeof(glm::vec3), color, pointSize,
        state->getOutPose()->m_skeleton->m_name);

    DebugDrawHandle linesHandle = drawLinesUniformColor(
        &lines[0].x, lineCounter * sizeof(glm::vec3), color, pointSize,
        state->getOutPose()->m_skeleton->m_name);

    // lets prepare the compound handle
    // there are two items only lines and points and the points is the first
    DebugTracker tracker{};
    tracker.compoundCount = 2;
    // TODO remove this naked allocation use an allocator
    // being compound we only store the compound count and handles
    tracker.compoundHandles = new DebugDrawHandle[2];
    tracker.compoundHandles[0] = pointsHandle;
    tracker.compoundHandles[1] = linesHandle;

    const uint32_t compoundBit = 1u << 31u;
    const DebugDrawHandle returnHandle{compoundBit |
                                       (MAGIC_NUMBER_COUNTER << 16) | 0};

    m_trackers[returnHandle.handle] = tracker;

    ++MAGIC_NUMBER_COUNTER;

    return returnHandle;
  }
}  // namespace SirEngine::dx12

void Dx12DebugRenderer::renderQueue(
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
    dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

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
        currentFc->commandList->IASetVertexBuffers(0, 1, nullptr);
        currentFc->commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      } else {
        currentFc->commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_LINELIST);
      }
      commandList->SetGraphicsRootDescriptorTable(2, prim.srv.gpuHandle);

      currentFc->commandList->DrawInstanced(prim.primitiveToRender, 1, 0, 0);
    }
    annotateGraphicsEnd();
  }
}

void Dx12DebugRenderer::render(const TextureHandle input,
                               const TextureHandle depth) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  // first static stuff
  renderQueue(m_renderables, input, depth);

  // TODO fix this, every draw call should set as appropriate
  currentFc->commandList->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}  // namespace SirEngine::dx12
void Dx12DebugRenderer::clearUploadRequests() {
  const uint64_t id = GLOBAL_FENCE->GetCompletedValue();

  const int requestSize = static_cast<int>(m_uploadRequests.size()) - 1;
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
inline int push3toVec(float *data, float x, float y, float z, int counter) {
  data[counter++] = x;
  data[counter++] = y;
  data[counter++] = z;

  return counter;
}
inline int push4toVec(float *data, float x, float y, float z, int counter) {
  data[counter++] = x;
  data[counter++] = y;
  data[counter++] = z;
  data[counter++] = 1.0f;

  return counter;
}
inline int push3toVec(float *data, const glm::vec4 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;

  return counter;
}
inline int push3toVec(float *data, const glm::vec3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;

  return counter;
}
inline int push4toVec(float *data, const glm::vec3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;
  data[counter++] = 1.0f;

  return counter;
}
int drawSquareBetweenTwoPoints(float *data, const glm::vec3 minP,
                               const glm::vec3 maxP, const float y,
                               int counter) {
  counter = push4toVec(data, minP.x, y, minP.z, counter);
  counter = push4toVec(data, maxP.x, y, minP.z, counter);

  counter = push4toVec(data, maxP.x, y, minP.z, counter);
  counter = push4toVec(data, maxP.x, y, maxP.z, counter);

  counter = push4toVec(data, maxP.x, y, maxP.z, counter);
  counter = push4toVec(data, minP.x, y, maxP.z, counter);

  counter = push4toVec(data, minP.x, y, maxP.z, counter);
  counter = push4toVec(data, minP.x, y, minP.z, counter);
  return counter;
}

DebugDrawHandle Dx12DebugRenderer::drawBoundingBoxes(BoundingBox *data,
                                                     int count,
                                                     const glm::vec4 color,
                                                     const char *debugName) {
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4 vertical
  // two is because we need two points per line, we are not doing trianglestrip
  int totalSize = 4 * count * 12 * 2;  // here 4 is the xmfloat4

  auto *points = reinterpret_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec4) * count * 12 * 2));
  int counter = 0;
  for (int i = 0; i < count; ++i) {
    assert(counter <= totalSize);
    const auto &minP = data[i].min;
    const auto &maxP = data[i].max;
    counter = drawSquareBetweenTwoPoints(points, minP, maxP, minP.y, counter);
    counter = drawSquareBetweenTwoPoints(points, minP, maxP, maxP.y, counter);

    // draw vertical lines
    counter = push4toVec(points, minP, counter);
    counter = push4toVec(points, minP.x, maxP.y, minP.z, counter);
    counter = push4toVec(points, maxP.x, minP.y, minP.z, counter);
    counter = push4toVec(points, maxP.x, maxP.y, minP.z, counter);

    counter = push4toVec(points, maxP.x, minP.y, maxP.z, counter);
    counter = push4toVec(points, maxP.x, maxP.y, maxP.z, counter);

    counter = push4toVec(points, minP.x, minP.y, maxP.z, counter);
    counter = push4toVec(points, minP.x, maxP.y, maxP.z, counter);
    assert(counter <= totalSize);
  }
  drawLinesUniformColor(points, totalSize * sizeof(float), color,
                        static_cast<float>(totalSize), debugName);

  // this is not compound;
  const int compoundBit = 0;
  const DebugDrawHandle returnHandle{compoundBit |
                                     (MAGIC_NUMBER_COUNTER << 16) | 0};
  ++MAGIC_NUMBER_COUNTER;
  return returnHandle;
}

DebugDrawHandle Dx12DebugRenderer::drawAnimatedBoundingBoxes(
    DebugDrawHandle handle, BoundingBox *data, int count, glm::vec4 color,
    const char *debugName) {
  // first get AABB data
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4 vertical
  // two is because we need two points per line, we are not doing trianglestrip
  int totalSize = 3 * count * 12 * 2;  // here 3 is the xmfloat3

  auto *points = reinterpret_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec3) * count * 12 * 2));
  int counter = 0;
  for (int i = 0; i < count; ++i) {
    assert(counter <= totalSize);
    const auto &minP = data[i].min;
    const auto &maxP = data[i].max;
    counter = drawSquareBetweenTwoPoints(points, minP, maxP, minP.y, counter);
    counter = drawSquareBetweenTwoPoints(points, minP, maxP, maxP.y, counter);

    // draw vertical lines
    counter = push3toVec(points, minP, counter);
    counter = push3toVec(points, minP.x, maxP.y, minP.z, counter);
    counter = push3toVec(points, maxP.x, minP.y, minP.z, counter);
    counter = push3toVec(points, maxP.x, maxP.y, minP.z, counter);

    counter = push3toVec(points, maxP.x, minP.y, maxP.z, counter);
    counter = push3toVec(points, maxP.x, maxP.y, maxP.z, counter);

    counter = push3toVec(points, minP.x, minP.y, maxP.z, counter);
    counter = push3toVec(points, minP.x, maxP.y, maxP.z, counter);
    assert(counter <= totalSize);
  }

  if (handle.isHandleValid()) {
    // lets get the trackers out for each one
    const auto found = m_trackers.find(handle.handle);
    assert(found != m_trackers.end());
    assert(found->second.compoundCount == 0);

    const DebugTracker &tracker = found->second;
    assert(tracker.sizeInBtye == (sizeof(float) * totalSize));
    memcpy(tracker.mappedData, points, tracker.sizeInBtye);
    return handle;

  } else {
    const DebugDrawHandle outHandle =
        drawLinesUniformColor(points, totalSize * sizeof(float), color,
                              static_cast<float>(totalSize), debugName);

    return outHandle;
  }
}

DebugDrawHandle Dx12DebugRenderer::drawAnimatedBoundingBoxFromFullPoints(
    const DebugDrawHandle handle, const glm::vec3 *data, const int count,
    const glm::vec4 color, const char *debugName) {
  // first get AABB data
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4 vertical
  // two is because we need two points per line, we are not doing trianglestrip
  const int totalSize = 4 * count * 12 * 2;  // here 4 is the xmfloat3

  auto *points = reinterpret_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec4) * count * 12 * 2));
  int counter = 0;

  // draw vertical lines
  counter = push4toVec(points, data[0], counter);
  counter = push4toVec(points, data[2], counter);
  counter = push4toVec(points, data[0], counter);
  counter = push4toVec(points, data[3], counter);
  counter = push4toVec(points, data[0], counter);
  counter = push4toVec(points, data[4], counter);
  counter = push4toVec(points, data[2], counter);
  counter = push4toVec(points, data[6], counter);
  counter = push4toVec(points, data[4], counter);
  counter = push4toVec(points, data[6], counter);
  counter = push4toVec(points, data[2], counter);
  counter = push4toVec(points, data[5], counter);
  counter = push4toVec(points, data[3], counter);
  counter = push4toVec(points, data[7], counter);
  counter = push4toVec(points, data[3], counter);
  counter = push4toVec(points, data[5], counter);
  counter = push4toVec(points, data[1], counter);
  counter = push4toVec(points, data[6], counter);
  counter = push4toVec(points, data[1], counter);
  counter = push4toVec(points, data[5], counter);
  counter = push4toVec(points, data[1], counter);
  counter = push4toVec(points, data[7], counter);
  counter = push4toVec(points, data[4], counter);
  counter = push4toVec(points, data[7], counter);

  assert(counter <= totalSize);

  if (handle.isHandleValid()) {
    // lets get the trackers out for each one
    const auto found = m_trackers.find(handle.handle);
    assert(found != m_trackers.end());
    assert(found->second.compoundCount == 0);

    const DebugTracker &tracker = found->second;
    assert(tracker.sizeInBtye == (sizeof(float) * totalSize));
    memcpy(tracker.mappedData, points, tracker.sizeInBtye);
    return handle;

  } else {
    const DebugDrawHandle outHandle =
        drawLinesUniformColor(points, totalSize * sizeof(float), color,
                              static_cast<float>(totalSize), debugName);

    return outHandle;
  }
}  // namespace SirEngine::dx12

void Dx12DebugRenderer::drawMatrix(const glm::mat4 &mat, float size,
                                   glm::vec4 color, const char *debugName) {
  const int totalSize =
      4 * 2 * 3;  // 3 axis, each with two points, 4 floats each point
  auto *points = reinterpret_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(float) * totalSize));

  int counter = 0;
  // start with z axis
  auto scaledZ = mat[2] * (size * 2.5f);
  auto movedPosZ = mat[3] + scaledZ;
  counter = push4toVec(points, mat[3], counter);
  counter = push4toVec(points, movedPosZ, counter);

  auto scaledX = mat[0] * size;
  auto movedPosX = mat[3] + scaledX;
  counter = push4toVec(points, mat[3], counter);
  counter = push4toVec(points, movedPosX, counter);

  auto scaledY = (mat[1] * (size * 1.5f));
  auto movedPosY = mat[3] + scaledY;
  counter = push4toVec(points, mat[3], counter);
  counter = push4toVec(points, movedPosY, counter);

  drawLinesUniformColor(points, totalSize * sizeof(float), color, totalSize,
                        debugName);
}
}  // namespace SirEngine::dx12