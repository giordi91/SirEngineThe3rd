#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class AssetManagerNode final : public GNode {
public:
  enum PLUGS { ASSET_STREAM = OUTPUT_PLUG_CODE(0), COUNT = 1 };

public:
  AssetManagerNode(GraphAllocators &allocators);
  virtual ~AssetManagerNode() = default;
  void compute() override;
};

} // namespace SirEngine
