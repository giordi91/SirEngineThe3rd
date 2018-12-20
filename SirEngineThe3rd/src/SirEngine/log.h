#pragma once

#include "core.h"
#include "spdlog/spdlog.h"

namespace SirEngine {

class Log {
public:
  static void SIR_ENGINE_API init();
  inline static SIR_ENGINE_API std::shared_ptr<spdlog::logger> &
  getCoreLogger() {
    return s_coreLogger;
  }
  inline static SIR_ENGINE_API std::shared_ptr<spdlog::logger> &
  getClientLogger() {
    return s_clientLogger;
  }

private:
  static std::shared_ptr<spdlog::logger> s_coreLogger;
  static std::shared_ptr<spdlog::logger> s_clientLogger;
};
} // namespace SirEngine

#define SE_CORE_TRACE(...) ::SirEngine::Log::getCoreLogger()->trace(__VA_ARGS__)
#define SE_CORE_INFO(...) ::SirEngine::Log::getCoreLogger()->info(__VA_ARGS__)
#define SE_CORE_WARN(...) ::SirEngine::Log::getCoreLogger()->warn(__VA_ARGS__)
#define SE_CORE_ERROR(...) ::SirEngine::Log::getCoreLogger()->error(__VA_ARGS__)
#define SE_CORE_FATAL(...) ::SirEngine::Log::getCoreLogger()->fatal(__VA_ARGS__)

#define SE_TRACE(...) ::SirEngine::Log::getClientLogger()->trace(__VA_ARGS__)
#define SE_INFO(...) ::SirEngine::Log::getClientLogger()->info(__VA_ARGS__)
#define SE_WARN(...) ::SirEngine::Log::getClientLogger()->warn(__VA_ARGS__)
#define SE_ERROR(...) ::SirEngine::Log::getClientLogger()->error(__VA_ARGS__)
#define SE_FATAL(...) ::SirEngine::Log::getClientLogger()->fatal(__VA_ARGS__)
