#include "SirEngine/graphics/graphicsCore.h"
#include "SirEngine/Window.h"

// DX12 implementation
#if GRAPHICS_API == DX12
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/swapChain.h"
#endif
namespace SirEngine {
namespace graphics {

#if GRAPHICS_API == DX12
bool initializeGraphics(Window *wnd, uint32_t width, uint32_t height) {
  return dx12::initializeGraphicsDx12(wnd, width, height);
}

void onResize(const uint32_t width, const uint32_t height) {
  dx12::SWAP_CHAIN->resize(&dx12::CURRENT_FRAME_RESOURCE->fc, width, height);
}
void newFrame() { dx12::newFrameDx12(); }
void dispatchFrame() { dx12::dispatchFrameDx12(); }
void stopGraphics() { dx12::stopGraphicsDx12(); }
void shutdownGraphics() { dx12::shutdownGraphicsDx12(); }

void beginHeadlessWork() { dx12::beginHeadlessWorkDx12(); }
void endHeadlessWork() { dx12::endHeadlessWorkDx12(); }

void flush() { dx12::flushDx12(); }
#endif
} // namespace graphics
} // namespace SirEngine
