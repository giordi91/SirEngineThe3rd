#pragma once
#include <cstdint>

namespace SirEngine {
class Window;
namespace graphics {

bool initializeGraphics(Window *wnd, uint32_t width, uint32_t height);
// should be called from the application whenever there is a resize
void onResize(uint32_t width, uint32_t height);

// This function should be called before the evaluation of the
// layer stack, at this point, all the initialization that must have
// happened did happened, lock on frame resources, reset of command list
// if using low level API etc
void newFrame();

// This function should be called after all the layers have been evaluated,
// this will dispatch whatever is left, blit the final frame to the frame buffer
// etc
void dispatchFrame();

// this function should be called when we want to exit, this function will just
// make sure that the graphic work is done, so that then shutdown can be called
// when convenient
void stopGraphics();
void shutdownGraphics();

//functions for headless client
void beginHeadlessWork();
void endHeadlessWork();

void flush();
} // namespace graphics

} // namespace SirEngine
