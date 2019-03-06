#pragma once
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <vector>

namespace SirEngine {

class PostProcessEffect {
public:
  PostProcessEffect(const char *name, const char* type) : m_name(name), m_type(type) {}
  virtual ~PostProcessEffect() = default;
  virtual void initialize() = 0;
  virtual void render(TextureHandle input, TextureHandle output) = 0;
  virtual void clear() = 0;
  inline const char *getName() const { return m_name; }
  inline void setEnable(const bool enabled) { m_enabled = enabled; }
  inline bool isEnabled() const { return m_enabled; }
  inline const char* getType() const {return m_type;}

protected:
  const char *m_name;
  const char* m_type;
  bool m_enabled = true;
};

class PostProcessStack final : public GraphNode {
public:
  PostProcessStack();
  virtual void initialize() override;
  void clear() override;
  virtual void compute() override;
  virtual void resize(int screenWidth, int screenHeight) override;
  inline void registerPassToStack(PostProcessEffect *pass) {
    m_stack.push_back(pass);
  };
  template <typename T> T *allocateRenderPass(const char *name) {
    T *pass = new T(name);
    m_stack.push_back(pass);
    return pass;
  }
  const std::vector<PostProcessEffect *> &getEffects() const { return m_stack; }

private:
  std::vector<PostProcessEffect *> m_stack;
  std::vector<TextureHandle> m_buffers;
  TextureHandle handles[2];
  int m_internalCounter = 0;
};

} // namespace SirEngine
