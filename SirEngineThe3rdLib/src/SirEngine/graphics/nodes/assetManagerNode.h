#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class AssetManagerNode final : public GraphNode
{
public:
	AssetManagerNode();
	virtual ~AssetManagerNode()=default;
	void compute() override;
};

} // namespace SirEngine
