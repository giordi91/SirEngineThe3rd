#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class GBufferPassPBR final : public GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = outputPlugCode(0),
    NORMALS_RT = outputPlugCode(1),
    SPECULAR_RT = outputPlugCode(2),
    DEPTH_RT = outputPlugCode(3),
    COUNT = 4
  };

public:
  explicit GBufferPassPBR(GraphAllocators &allocators);
  virtual ~GBufferPassPBR() { clear(); };
  void initialize() override;
  void compute() override;
  void clear() override;
  void onResizeEvent(int screenWidth, int screenHeight) override;
  void populateNodePorts() override;

private:
  TextureHandle m_geometryBuffer{};
  TextureHandle m_normalBuffer{};
  TextureHandle m_specularBuffer{};
  TextureHandle m_depth{};
  BufferBindingsHandle m_bindHandle;
};

} // namespace SirEngine
