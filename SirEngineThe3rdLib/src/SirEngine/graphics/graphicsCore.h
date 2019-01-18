#pragma once

namespace SirEngine {
namespace graphics {

// should be called from the application whenever there is a resize
void onResize(unsigned int width, unsigned int height);

// This function should be called before the evaluation of the
// layer stack, at this point, all the initialization that must have
// happened did happened, lock on frame resources, reset of command list
// if using low level api etc
void newFrame();

// This function should be called after all the layers have been evaluated,
// this will dispatch whatever is left, blit the final frame to the frame buffer
// etc
void dispatchFrame();
} // namespace graphics

} // namespace SirEngine
