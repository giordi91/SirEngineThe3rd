#pragma once

namespace SirEngine {
inline void setFlagBitfield(uint32_t& flags, uint32_t bitToSet,
                            bool valueToSet) {
  // from
  // https://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit
  // this allows us to set in a single go without the branch for using |= or &=
  int valueToSetInt = static_cast<int>(valueToSet);
  flags ^= (-valueToSetInt ^ flags) & bitToSet;
}

inline bool isFlagSet(uint32_t flags, uint32_t flagToCheck) {
  return (flags & flagToCheck) > 0;
}
enum EngineFlagsInteraction : uint32_t {
  EngineFlagsInteraction_None = 0,
  EngineFlagsInteraction_AllowInput = 1,
  EngineFlagsInteraction_ViewportFocused = 2,
  EngineFlagsInteraction_ViewportHovered = 4,
};

struct EngineFlags {
  uint32_t m_interaction{};
};

}  // namespace SirEngine