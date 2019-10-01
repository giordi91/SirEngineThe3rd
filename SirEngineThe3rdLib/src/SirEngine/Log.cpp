#include "log.h"
#include <iostream>

namespace SirEngine {

	std::shared_ptr<spdlog::logger> Log::s_coreLogger;
	std::shared_ptr<spdlog::logger> Log::s_clientLogger;

	void Log::init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_coreLogger = spdlog::stdout_color_mt("SirEngine");
		s_coreLogger->set_level(spdlog::level::trace);
		s_clientLogger= spdlog::stdout_color_mt("APP");
		s_clientLogger->set_level(spdlog::level::trace);
	}

	void Log::free()
	{

		spdlog::drop(s_coreLogger->name());
		spdlog::drop(s_clientLogger->name());
		s_coreLogger.reset();
		s_clientLogger.reset();
		s_coreLogger = nullptr;
		s_clientLogger= nullptr;
	}
} // namespace SirEngine
