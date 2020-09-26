#pragma once
#include "SirEngine/core.h"

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
  inline bool isKeyReleased(const uint32_t input) const {
    return (m_keys[input] == 0) & (m_keysPrev[input] != 0);
  }
  inline bool isKeyPressedThisFrame(const uint32_t input) const {
    return (m_keys[input] != 0) & (m_keysPrev[input] == 0);
  }
  // NOTE: this is not a proper key repeat event, just tells you if
  // the key was pressed in the previous frame and this frame, which
  // might be most likely the case with every press since a user cannot
  // press that fast
  inline bool isKeyRepeaded(const uint32_t input) const {
    return (m_keys[input] != 0) & (m_keysPrev[input] != 0);
  }

  inline uint32_t isKeyDownPreviousFrame(const uint32_t input) const {
    return m_keysPrev[input];
  }
  inline void setMouse(const MOUSE_BUTTONS button, const uint32_t input) {
    m_mouse[button] = input;
  }
  inline void setMousePos(const float x, const float y) {
    m_mousePosX = x;
    m_mousePosY = y;
  }
  void swapFrameKey() {
    // copying current keys to the previous frame
    memcpy(m_keysPrev, m_keys, sizeof(uint32_t) * SIZE_OF_KEYS);
  };

  int m_mouse[4]{};
  float m_mousePosX = 0;
  float m_mousePosY = 0;
  uint32_t m_keys[SIZE_OF_KEYS]{};
  uint32_t m_keysPrev[SIZE_OF_KEYS]{};
  bool m_uiCapturingMouse = false;
  bool m_uiCapturingKeyboard = false;
};
}  // namespace SirEngine
