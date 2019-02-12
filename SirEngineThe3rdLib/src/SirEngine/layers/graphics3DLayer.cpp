#include "SirEngine/layers/graphics3DLayer.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/swapChain.h"
#include <DirectXMath.h>

#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/nodes/assetManagerNode.h"
#include "SirEngine/graphics/nodes/simpleForward.h"
namespace SirEngine {

void Graphics3DLayer::onAttach() {
  globals::MAIN_CAMERA = new Camera3DPivot();
  // globals::MAIN_CAMERA->setLookAt(0, 125, 0);
  // globals::MAIN_CAMERA->setPosition(00, 125, 60);

  globals::MAIN_CAMERA->setLookAt(0, 0, 0);
  globals::MAIN_CAMERA->setPosition(00, 0, 5);
  globals::MAIN_CAMERA->updateCamera();

  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;

  if (!currentFc->isListOpen) {
    dx12::resetAllocatorAndList(currentFc);
  }

  m_shaderManager = new SirEngine::dx12::ShaderManager();
  m_shaderManager->init();
  m_shaderManager->loadShadersInFolder("data/processed/shaders/rasterization");

  m_root = new dx12::RootSignatureManager();
  m_root->loadSingaturesInFolder("data/processed/rs");

  m_reg = new dx12::ShadersLayoutRegistry();

  m_pso = new temp::rendering::PSOManager();
  m_pso->init(dx12::DEVICE, m_reg, m_root, m_shaderManager);
  m_pso->loadPSOInFolder("data/pso");

  // ask for the camera buffer handle;
  m_cameraHandle = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(dx12::CameraBuffer));


  sphereH = globals::ASSET_MANAGER->loadAsset("data/assets/sphere.json");
  globals::ASSET_MANAGER->loadAsset("data/assets/sphere.json");
  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE, currentFc);
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);

  dx12::RENDERING_GRAPH = new Graph();

  auto assetNode = new AssetManagerNode();
  auto finalBlit = new FinalBlitNode();
  auto simpleForward = new SimpleForward("simpleForward");

  // temporary graph for testing
  dx12::RENDERING_GRAPH->addNode(assetNode);
  dx12::RENDERING_GRAPH->addNode(finalBlit);
  dx12::RENDERING_GRAPH->addNode(simpleForward);
  dx12::RENDERING_GRAPH->setFinalNode(finalBlit);
  dx12::RENDERING_GRAPH->connectNodes(assetNode, "matrices", simpleForward,
                                      "matrices");
  dx12::RENDERING_GRAPH->connectNodes(assetNode, "meshes", simpleForward,
                                      "meshes");
  dx12::RENDERING_GRAPH->connectNodes(assetNode, "materials", simpleForward,
                                      "materials");
  dx12::RENDERING_GRAPH->connectNodes(simpleForward, "outTexture", finalBlit,
                                      "inTexture");
  dx12::RENDERING_GRAPH->finalizeGraph();
}
void Graphics3DLayer::onDetach() {}
void Graphics3DLayer::onUpdate() {

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  if (!currentFc->isListOpen) {
    dx12::resetAllocatorAndList(currentFc);
  }

  auto commandList = currentFc->commandList;
  commandList->RSSetViewports(1, dx12::SWAP_CHAIN->getViewport());
  commandList->RSSetScissorRects(1, dx12::SWAP_CHAIN->getScissorRect());
  dx12::SWAP_CHAIN->clearDepth();
  auto back = dx12::SWAP_CHAIN->currentBackBufferView();
  auto depth = dx12::SWAP_CHAIN->getDepthCPUDescriptor();

  commandList->OMSetRenderTargets(1, &back, true, &depth);

  globals::MAIN_CAMERA->updateCamera();
  m_camBufferCPU.vFov = 60.0f;
  m_camBufferCPU.screenWidth = static_cast<float>(globals::SCREEN_WIDTH);
  m_camBufferCPU.screenHeight = static_cast<float>(globals::SCREEN_HEIGHT);
  m_camBufferCPU.mvp = DirectX::XMMatrixTranspose(
      globals::MAIN_CAMERA->getMVP(DirectX::XMMatrixIdentity()));

  globals::CONSTANT_BUFFER_MANAGER->updateConstantBuffer(m_cameraHandle,
                                                      &m_camBufferCPU);

  auto *pso = m_pso->getComputePSOByName("simpleMeshPSOTex");
  commandList->SetPipelineState(pso);
  auto *rs = m_root->getRootSignatureFromName("simpleMeshRSTex");
  commandList->SetGraphicsRootSignature(rs);

  commandList->SetGraphicsRootDescriptorTable(
      0,
	  //TODO remove this, wrap it into a context maybe and remove graphics core?
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(m_cameraHandle)
          .gpuHandle);

  dx12::RENDERING_GRAPH->compute();

  // making any clean up for the mesh manager if we have to
  dx12::MESH_MANAGER->clearUploadRequests();
}
void Graphics3DLayer::onEvent(Event &event) {

  EventDispatcher dispatcher(event);
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseMoveEvent));
}

void Graphics3DLayer::clear() {}

bool Graphics3DLayer::onMouseButtonPressEvent(MouseButtonPressEvent &e) {
  if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::LEFT) {
    leftDown = true;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::RIGHT) {
    rightDown = true;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::MIDDLE) {
    middleDown = true;
    return true;
  }
  return false;
}

bool Graphics3DLayer::onMouseButtonReleaseEvent(MouseButtonReleaseEvent &e) {
  if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::LEFT) {
    leftDown = false;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::RIGHT) {
    rightDown = false;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::MIDDLE) {
    middleDown = false;
    return true;
  }
  return false;
}

bool Graphics3DLayer::onMouseMoveEvent(MouseMoveEvent &e) {
  const float deltaX = previousX - e.getX();
  const float deltaY = previousY - e.getY();
  if (leftDown) {
    globals::MAIN_CAMERA->rotCamera(deltaX, deltaY);
  } else if (middleDown) {
    globals::MAIN_CAMERA->panCamera(deltaX, deltaY);
  } else if (rightDown) {
    globals::MAIN_CAMERA->zoomCamera(deltaX);
  }

  // storing old position
  previousX = e.getX();
  previousY = e.getY();
  return true;
}

} // namespace SirEngine
