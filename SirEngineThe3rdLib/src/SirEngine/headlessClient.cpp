#include "SirEngine/headlessClient.h"
#include "platform/windows/graphics/dx12/DX12.h"

namespace SirEngine {
HeadlessClient::HeadlessClient() {
	//TODO fix headless client
  //graphics::initializeGraphics(nullptr, 0, 0);
}

HeadlessClient::~HeadlessClient() { //graphics::shutdownGraphics();
	}

void HeadlessClient::beginWork() { //graphics::beginHeadlessWork();
	}
void HeadlessClient::endWork() { //graphics::endHeadlessWork();
	}
void HeadlessClient::flushAllOperation() { //graphics::flush();
	}

dx12::Dx12RootSignatureManager *HeadlessClient::getRootSignatureManager() {
  return dx12::ROOT_SIGNATURE_MANAGER;
}

dx12::Dx12PSOManager *HeadlessClient::getPSOManager() { return dx12::PSO_MANAGER; }

dx12::Dx12TextureManager *HeadlessClient::getTextureManager() {
  return dx12::TEXTURE_MANAGER;
}

dx12::FrameResource *HeadlessClient::getFrameResource() {
  return dx12::CURRENT_FRAME_RESOURCE;
}

dx12::DescriptorHeap *HeadlessClient::getCbvSrvUavHeap() {
  return dx12::GLOBAL_CBV_SRV_UAV_HEAP;
}

ID3D12Device *HeadlessClient::getDevice() { return dx12::DEVICE; }

ID3D12CommandQueue *HeadlessClient::getQueue() {
  return dx12::GLOBAL_COMMAND_QUEUE;
}
} // namespace SirEngine
