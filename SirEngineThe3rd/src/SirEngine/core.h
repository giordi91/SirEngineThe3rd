#pragma once

#ifdef SE_PLATFORM_WINDOWS
#ifdef SE_BUILD_DLL
#define SIR_ENGINE_API __declspec(dllexport)
#else
#define SIR_ENGINE_API __declspec(dllimport)
#endif
#else
#error Sir Engine only runs on windows
#endif
