#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>

namespace SirEngine {

class GBufferPassPBR final : public GraphNode {
public:
	explicit GBufferPassPBR(const char *name);
  virtual ~GBufferPassPBR() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

private:
  TextureHandle m_geometryBuffer{};
  TextureHandle m_normalBuffer{};
  TextureHandle m_specularBuffer{};
  TextureHandle m_depth{};
};

} // namespace SirEngine
