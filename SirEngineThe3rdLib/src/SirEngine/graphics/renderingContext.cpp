#include "renderingContext.h"

#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#if BUILD_DX12
#include "platform/windows/graphics/dx12/DX12.h"
#endif

#if BUILD_VK
#include "platform/windows/graphics/vk/vk.h"
#endif

namespace SirEngine {

RenderingContext *
createWindowsRenderingContext(const RenderingContextCreationSettings &settings,
                              uint32_t width, uint32_t height) {
  if (!RenderingContext::isApiSupported(settings.graphicsAPI)) {
    // TODO convert code to name
    SE_CORE_ERROR("Requested api is not supported on system, API code: {0}",
                  static_cast<uint32_t>(settings.graphicsAPI));
    return nullptr;
  }

  // API is supported we can create the context based on the given API
  switch (settings.graphicsAPI) {
  case (GRAPHIC_API::DX12): {
#if BUILD_DX12
    return dx12::createDx12RenderingContext(settings, width, height);
#else
    SE_CORE_ERROR("Asking for a DX12 rendering context but DX12 was not "
                  "enabled at compile time");
    exit(-1);
#endif
  }
  case GRAPHIC_API::VULKAN:;
#if BUILD_VK
    return vk::createVkRenderingContext(settings, width, height);
#else
    SE_CORE_ERROR("Asking for a Vulkan rendering context but vulkan was not "
                  "enabled at compile time");
    exit(-1);
#endif
  default:;
    assert(!"Not supported API requested");
    return nullptr;
  }
}

RenderingContext *
RenderingContext::create(const RenderingContextCreationSettings &settings,
                         uint32_t width, uint32_t height) {
  return createWindowsRenderingContext(settings, width, height);
}

bool RenderingContext::isApiSupported(const GRAPHIC_API graphicsApi) {
  return (graphicsApi == GRAPHIC_API::DX12) |
         (graphicsApi == GRAPHIC_API::VULKAN);
}

/*
void RenderingContext::initialize() {
  // ask for the camera buffer handle;
  m_cameraHandle =
      globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(CameraBuffer));

  float intensity = 4.0f;
  m_light.lightColor = {intensity, intensity, intensity, 1.0f};
  // m_light.lightDir = {0.0f, 0.0f, -1.0f, 0.0f};
  m_light.lightDir = {-1.0f, -0.6f, -1.0f, 1.0f};
  m_light.lightPosition = {10.0f, 10.0f, 10.0f, 1.0f};

  // build a look at matrix for the light
  auto tempView = DirectX::XMLoadFloat4(&m_light.lightDir);
  tempView.m128_f32[3] = 0.0f;
  const auto viewDir = DirectX::XMVector3Normalize(tempView);
  const DirectX::XMVECTOR upVector =
      DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0);

  const auto cross = DirectX::XMVector3Cross(upVector, viewDir);
  const auto crossNorm = DirectX::XMVector3Normalize(cross);

  const auto newUp = DirectX::XMVector3Cross(viewDir, crossNorm);
  const auto newUpNorm = DirectX::XMVector3Normalize(newUp);

  m_light.localToWorld =
      DirectX::XMMATRIX(crossNorm, newUpNorm, viewDir,
                        DirectX::XMLoadFloat4(&m_light.lightPosition));

  auto det = DirectX::XMMatrixDeterminant(m_light.localToWorld);
  m_light.worldToLocal = DirectX::XMMatrixInverse(&det, m_light.localToWorld);

  // allocate the constant buffer
  m_lightCB = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(DirectionalLightData), &m_light);
}
void RenderingContext::setupCameraForFrame() {
  globals::MAIN_CAMERA->updateCamera();
  m_camBufferCPU.vFov = 60.0f;
  m_camBufferCPU.screenWidth = static_cast<float>(globals::SCREEN_WIDTH);
  m_camBufferCPU.screenHeight = static_cast<float>(globals::SCREEN_HEIGHT);
  m_camBufferCPU.MVP = DirectX::XMMatrixTranspose(
      globals::MAIN_CAMERA->getMVP(DirectX::XMMatrixIdentity()));
  m_camBufferCPU.ViewMatrix = DirectX::XMMatrixTranspose(
      globals::MAIN_CAMERA->getViewInverse(DirectX::XMMatrixIdentity()));
  m_camBufferCPU.VPinverse = DirectX::XMMatrixTranspose(
      globals::MAIN_CAMERA->getMVPInverse(DirectX::XMMatrixIdentity()));
  m_camBufferCPU.perspectiveValues = globals::MAIN_CAMERA->getProjParams();
  DirectX::XMFLOAT3 camPos = globals::MAIN_CAMERA->getPosition();
  m_camBufferCPU.position =
      DirectX::XMFLOAT4{camPos.x, camPos.y, camPos.z, 1.0f};

  globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferNotBuffered(
      m_cameraHandle, &m_camBufferCPU);
}

void RenderingContext::setupLightingForFrame() {
  globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferNotBuffered(m_lightCB,
                                                                    &m_light);
}

void RenderingContext::bindCameraBuffer(const int index) const {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  D3D12_GPU_DESCRIPTOR_HANDLE handle =
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(m_cameraHandle)
          .gpuHandle;
  commandList->SetGraphicsRootDescriptorTable(index, handle);
}
void RenderingContext::bindCameraBufferCompute(const int index) const {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->SetComputeRootDescriptorTable(
      index,
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(m_cameraHandle)
          .gpuHandle);
}

void RenderingContext::updateSceneBoundingBox() {
  const auto &boxes = dx12::MESH_MANAGER->getBoundingBoxes();
  const int boxesCount = static_cast<int>(boxes.size());
  float minX = std::numeric_limits<float>::max();
  float minY = minX;
  float minZ = minY;

  float maxX = std::numeric_limits<float>::min();
  float maxY = maxX;
  float maxZ = maxY;

  for (int i = 0; i < boxesCount; ++i) {
    const BoundingBox &box = boxes[i];
    // lets us compute bounding box
    minX = box.min.x < minX ? box.min.x : minX;
    minY = box.min.y < minY ? box.min.y : minY;
    minZ = box.min.z < minZ ? box.min.z : minZ;

    maxX = box.min.x > maxX ? box.min.x : maxX;
    maxY = box.min.y > maxY ? box.min.y : maxY;
    maxZ = box.min.z > maxZ ? box.min.z : maxZ;

    minX = box.max.x < minX ? box.max.x : minX;
    minY = box.max.y < minY ? box.max.y : minY;
    minZ = box.max.z < minZ ? box.max.z : minZ;

    maxX = box.max.x > maxX ? box.max.x : maxX;
    maxY = box.max.y > maxY ? box.max.y : maxY;
    maxZ = box.max.z > maxZ ? box.max.z : maxZ;
  }
  m_boundingBox.min.x = minX;
  m_boundingBox.min.y = minY;
  m_boundingBox.min.z = minZ;
  m_boundingBox.max.x = maxX;
  m_boundingBox.max.y = maxY;
  m_boundingBox.max.z = maxZ;
}

void expandBoundingBox(const BoundingBox &aabb, DirectX::XMFLOAT3 *points) {
  points[0] = aabb.min;
  points[1] = aabb.max;
  points[2] = DirectX::XMFLOAT3(points[0].x, points[0].y, points[1].z);
  points[3] = DirectX::XMFLOAT3(points[0].x, points[1].y, points[0].z);
  points[4] = DirectX::XMFLOAT3(points[1].x, points[0].y, points[0].z);
  points[5] = DirectX::XMFLOAT3(points[0].x, points[1].y, points[1].z);
  points[6] = DirectX::XMFLOAT3(points[1].x, points[0].y, points[1].z);
  points[7] = DirectX::XMFLOAT3(points[1].x, points[1].y, points[0].z);
}

void RenderingContext::updateDirectionalLightMatrix() {
  // we need to compute the scene bounding box in light space
  DirectX::XMFLOAT3 expanded[8];
  expandBoundingBox(m_boundingBox, expanded);
  float minX = std::numeric_limits<float>::max();
  float minY = minX;
  float minZ = minY;

  float maxX = std::numeric_limits<float>::min();
  float maxY = maxX;
  float maxZ = maxY;

  for (int i = 0; i < 8; ++i) {
    const DirectX::XMFLOAT3 &point = expanded[i];
    const DirectX::XMVECTOR point4 =
        DirectX::XMVectorSet(point.x, point.y, point.z, 1.0f);

    DirectX::XMVECTOR pp = DirectX::XMVectorSet(1, 1, 2, 1);
    const auto localPointV =
        DirectX::XMVector4Transform(point4, m_light.worldToLocal);

    DirectX::XMFLOAT4 localPoint;
    DirectX::XMStoreFloat4(&localPoint, localPointV);

    // lets us compute bounding box
    minX = localPoint.x < minX ? localPoint.x : minX;
    minY = localPoint.y < minY ? localPoint.y : minY;
    minZ = localPoint.z < minZ ? localPoint.z : minZ;

    maxX = localPoint.x > maxX ? localPoint.x : maxX;
    maxY = localPoint.y > maxY ? localPoint.y : maxY;
    maxZ = localPoint.z > maxZ ? localPoint.z : maxZ;
  }
  BoundingBox m_lightAABB;
  m_lightAABB.min.x = minX;
  m_lightAABB.min.y = minY;
  m_lightAABB.min.z = minZ;
  m_lightAABB.max.x = maxX;
  m_lightAABB.max.y = maxY;
  m_lightAABB.max.z = maxZ;

  // debug rendering of the light
  expandBoundingBox(m_lightAABB, expanded);

  for (int i = 0; i < 8; ++i) {
    DirectX::XMFLOAT3 &point = expanded[i];
    const DirectX::XMVECTOR point4 =
        DirectX::XMVectorSet(point.x, point.y, point.z, 1.0f);

    auto localPointV =
        DirectX::XMVector4Transform(point4, m_light.localToWorld);
    point.x = localPointV.m128_f32[0];
    point.y = localPointV.m128_f32[1];
    point.z = localPointV.m128_f32[2];
  }

  // we have the bounding box in light space we want to render it
  m_lightAABBHandle =
      dx12::DEBUG_RENDERER->drawAnimatedBoundingBoxFromFullPoints(
          m_lightAABBHandle, expanded, 1, DirectX::XMFLOAT4(1, 0, 0, 1), "");

  // we can now use min max to generate the projection matrix needed;
  const DirectX::XMMATRIX ortho =
      DirectX::XMMatrixOrthographicLH(maxX - minX, maxY - minY, maxZ, minZ);
  m_light.projectionMatrix = ortho;

  m_light.lightVP = DirectX::XMMatrixTranspose(
      DirectX::XMMatrixMultiply(m_light.worldToLocal, ortho));
}
*/
} // namespace SirEngine
