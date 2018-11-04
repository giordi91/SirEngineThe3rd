#pragma once

#include "core.h"
#include "spdlog/spdlog.h"
#include <memory>

namespace SirEngine {

class SIR_ENGINE_API Log {
public:
  static void init();
  inline static std::shared_ptr<spdlog::logger> &getCoreLogger() {
    return s_coreLogger;
  }
  inline static std::shared_ptr<spdlog::logger> &getClientLogger() {
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

#define SE_TRACE(...) ::SirEngine::Log::getCoreLogger()->trace(__VA_ARGS__)
#define SE_INFO(...) ::SirEngine::Log::getCoreLogger()->info(__VA_ARGS__)
#define SE_WARN(...) ::SirEngine::Log::getCoreLogger()->warn(__VA_ARGS__)
#define SE_ERROR(...) ::SirEngine::Log::getCoreLogger()->error(__VA_ARGS__)
#define SE_FATAL(...) ::SirEngine::Log::getCoreLogger()->fatal(__VA_ARGS__)
