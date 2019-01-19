#pragma once

#ifdef RC_PLATFORM_WINDOWS
#ifdef RC_BUILD_DLL
#define RC_API __declspec(dllexport)
#else
#define RC_API __declspec(dllimport)
#endif
#else
#error resource compiler only runs on windows 
#endif

#ifdef RC_BUILD_PLUGIN_DLL
#define RC_PLUGIN __declspec(dllexport)
#else
#define RC_PLUGIN __declspec(dllimport)
#endif
