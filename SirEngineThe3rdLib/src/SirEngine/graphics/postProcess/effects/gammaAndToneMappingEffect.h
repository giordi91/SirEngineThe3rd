#pragma once
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include <d3d12.h>

namespace  SirEngine
{
class GammaAndToneMappingEffect: public PostProcessEffect{
public:
  GammaAndToneMappingEffect(const char* name) : PostProcessEffect(name){}
  virtual ~GammaAndToneMappingEffect() = default;
  virtual void initialize() override;
  virtual void render(TextureHandle input, TextureHandle output) override;
  virtual void clear() override;
private:
	ID3D12RootSignature* rs;
	ID3D12PipelineState* pso;
};
	

}
