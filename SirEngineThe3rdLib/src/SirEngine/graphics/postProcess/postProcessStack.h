#pragma once
#include <vector>

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

struct PostProcessResources {
  TextureHandle depth;
};

class PostProcessEffect {
 public:
  PostProcessEffect(const char *name, const char *type)
      : m_name(name), m_type(type) {}
  virtual ~PostProcessEffect() = default;
  virtual void initialize() = 0;
  virtual void render(const TextureHandle input, const TextureHandle output,
                      const PostProcessResources &resources) = 0;
  virtual void clear() = 0;
  [[nodiscard]] inline const char *getName() const { return m_name; }
  inline void setEnable(const bool enabled) { m_enabled = enabled; }
  [[nodiscard]] inline bool isEnabled() const { return m_enabled; }
  [[nodiscard]] inline const char *getType() const { return m_type; }

 protected:
  const char *m_name;
  const char *m_type;
  bool m_enabled = true;
};

class PostProcessStack final : public GNode {
 public:
  enum PLUGS :uint32_t {
    IN_TEXTURE = inputPlugCode(0),
    DEPTH_RT = inputPlugCode(1),
    OUT_TEXTURE = outputPlugCode(0),
    COUNT = 3
  };

 public:
  explicit PostProcessStack(GraphAllocators &allocators);
  ~PostProcessStack() override = default;
  void initialize(CommandBufferHandle commandBuffer,RenderGraphContext* context) override;
  void initializeResolutionDepenantResources(CommandBufferHandle commandBuffer,RenderGraphContext* context) override;
  void clearResolutionDepenantResources() override;
  void clear() override;
  void compute(RenderGraphContext* context) override;
  void onResizeEvent(int screenWidth, int screenHeight,CommandBufferHandle commandBuffer,RenderGraphContext* context) override;
  inline void registerPassToStack(PostProcessEffect *pass) {
    m_stack.push_back(pass);
  };
  template <typename T>
  T *allocateRenderPass(const char *name) {
    T *pass = new T(name);
    m_stack.push_back(pass);
    return pass;
  }
  [[nodiscard]] const std::vector<PostProcessEffect *> &getEffects() const {
    return m_stack;
  }

  void populateNodePorts(RenderGraphContext* context) override;

 private:
  std::vector<PostProcessEffect *> m_stack;
  int m_internalCounter = 0;
  // handles
  TextureHandle inputRTHandle{};
  TextureHandle inputDepthHandle{};
  TextureHandle handles[2]{};
  BufferBindingsHandle m_bindHandles[2] = {};
};

}  // namespace SirEngine
