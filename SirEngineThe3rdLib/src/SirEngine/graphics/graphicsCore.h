#pragma once
#include <cstdint>

namespace SirEngine {
class Window;
namespace graphics {

bool initializeGraphics(Window* wnd, uint32_t width, uint32_t height);
// should be called from the application whenever there is a resize
void onResize(uint32_t width, uint32_t height);

// This function should be called before the evaluation of the
// layer stack, at this point, all the initialization that must have
// happened did happened, lock on frame resources, reset of command list
// if using low level api etc
void newFrame();

// This function should be called after all the layers have been evaluated,
// this will dispatch whatever is left, blit the final frame to the frame buffer
// etc
void dispatchFrame();

void stopGraphics();
void shutdownGraphics();
} // namespace graphics

} // namespace SirEngine
