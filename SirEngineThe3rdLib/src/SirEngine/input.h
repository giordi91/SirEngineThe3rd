#pragma once
#include <cstdint>
#include <cstring>

namespace SirEngine {
enum MOUSE_BUTTONS {
  LEFT = 0,
  RIGHT = 1,
  MIDDLE = 2,
  WHEEL = 3,

};
class Input final {

public:
  static constexpr int SIZE_OF_KEYS = 256;
  Input() = default;
  ~Input() = default;
  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;

  void init() {
    memset(m_keys, 0, sizeof(uint32_t) * SIZE_OF_KEYS);
    memset(m_mouse, 0, sizeof(uint32_t) * 4);
  };

  inline void keyDown(const uint32_t input) { m_keys[input] = 1u; }
  inline void keyUp(const uint32_t input) { m_keys[input] = 0u; }
  inline uint32_t isKeyDown(const uint32_t input) const {
    return m_keys[input];
  }
  inline void setMouse(const MOUSE_BUTTONS button, const uint32_t input) {
    m_mouse[button] = input;
  }
  inline void setMousePos(const float x, const float y) {
    m_mousePosX = x;
    m_mousePosY = y;
  }

  int m_mouse[4];
  float m_mousePosX = 0;
  float m_mousePosY = 0;
  uint32_t m_keys[SIZE_OF_KEYS];
};
} // namespace SirEngine
