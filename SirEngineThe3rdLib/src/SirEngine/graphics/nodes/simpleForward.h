#pragma once

#include <d3d12.h>
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class SimpleForward : public GraphNode {
 public:
  explicit SimpleForward(const char* name);
  virtual ~SimpleForward() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

 private:
  ID3D12RootSignature* rs = nullptr;
  PSOHandle pso;
};

}  // namespace SirEngine
