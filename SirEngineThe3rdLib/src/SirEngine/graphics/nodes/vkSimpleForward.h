#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "platform/windows/graphics/vk/volk.h"

#include "platform/windows/graphics/vk/vkTextureManager.h"

namespace SirEngine {


class VkSimpleForward final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 3
  };

public:
  explicit VkSimpleForward(GraphAllocators &allocators);
  virtual ~VkSimpleForward() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

  void populateNodePorts() override;
  void clear() override;
  vk::VkTexture2D  m_rt;

private:
  VkRenderPass m_pass;
  VkFramebuffer m_tempFrameBuffer;
  TextureHandle m_rtHandle{};
};

} // namespace SirEngine
