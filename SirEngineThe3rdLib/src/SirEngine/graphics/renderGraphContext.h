#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {
struct RenderGraphContext {
  // if null will render to render target
  TextureHandle m_renderTarget;
};

}  // namespace SirEngine
