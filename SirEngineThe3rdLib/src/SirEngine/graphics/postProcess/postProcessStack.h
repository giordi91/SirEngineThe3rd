#pragma once
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <vector>

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
  inline const char *getName() const { return m_name; }
  inline void setEnable(const bool enabled) { m_enabled = enabled; }
  inline bool isEnabled() const { return m_enabled; }
  inline const char *getType() const { return m_type; }

protected:
  const char *m_name;
  const char *m_type;
  bool m_enabled = true;
};

class PostProcessStack final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 3
  };

public:
  PostProcessStack(GraphAllocators &allocators);
  virtual ~PostProcessStack() = default;
  virtual void initialize() override;
  void clear() override;
  virtual void compute() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;
  inline void registerPassToStack(PostProcessEffect *pass) {
    m_stack.push_back(pass);
  };
  template <typename T> T *allocateRenderPass(const char *name) {
    T *pass = new T(name);
    m_stack.push_back(pass);
    return pass;
  }
  const std::vector<PostProcessEffect *> &getEffects() const { return m_stack; }

  void populateNodePorts() override;

private:
  std::vector<PostProcessEffect *> m_stack;
  std::vector<TextureHandle> m_buffers;
  int m_internalCounter = 0;
  //handles
  TextureHandle inputRTHandle{}; 
  TextureHandle inputDepthHandle{};
  TextureHandle handles[2]{};

};

} // namespace SirEngine
