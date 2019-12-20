#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class GBufferPassPBR final : public GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = OUTPUT_PLUG_CODE(0),
    NORMALS_RT = OUTPUT_PLUG_CODE(1),
    SPECULAR_RT = OUTPUT_PLUG_CODE(2),
    DEPTH_RT = OUTPUT_PLUG_CODE(3),
    COUNT = 4
  };

public:
  explicit GBufferPassPBR(GraphAllocators &allocators);
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
