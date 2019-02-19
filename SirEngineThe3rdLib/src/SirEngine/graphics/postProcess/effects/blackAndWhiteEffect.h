#pragma once
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include <d3d12.h>

namespace  SirEngine
{
class BlackAndWhiteEffect : public PostProcessEffect{
public:
  BlackAndWhiteEffect(const char* name) : PostProcessEffect(name){}
  virtual ~BlackAndWhiteEffect() = default;
  virtual void initialize() override;
  virtual void render(TextureHandle input, TextureHandle output) override;
  virtual void clear() override;
private:
	ID3D12RootSignature* rs;
	ID3D12PipelineState* pso;
};
	

}
