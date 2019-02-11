#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class AssetManagerNode : public GraphNode
{
public:
	AssetManagerNode();
	virtual ~AssetManagerNode()=default;
	virtual void compute() override;
};

} // namespace SirEngine
