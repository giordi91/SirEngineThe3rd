#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {
struct RenderGraphContext {
  // if null will render to render target
  TextureHandle m_renderTarget;
  // the size of the requested render, if a render target is provided it should
  // match if not, needs to match the swap chain, we provide it mostly to avoid
  // the user to have it to extract it from the render target handle and
  // validation
  uint32_t renderTargetWidth;
  uint32_t renderTargetHeight;
};

}  // namespace SirEngine
