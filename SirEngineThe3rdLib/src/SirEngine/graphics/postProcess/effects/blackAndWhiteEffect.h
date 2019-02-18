#pragma once
#include "SirEngine/graphics/postProcess/postProcessStack.h"

namespace  SirEngine
{
class BlackAndWhiteEffect : public PostProcessEffect{
public:
  BlackAndWhiteEffect() : PostProcessEffect("BlackAndWhiteEffect"){}
  virtual ~BlackAndWhiteEffect() = default;
  virtual void initialize() override;
  virtual void render(TextureHandle input, TextureHandle output) override;
  virtual void clear() override;
};
	

}
