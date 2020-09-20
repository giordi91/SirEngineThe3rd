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


//defined here such that I don't have to worry about including cstdint
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#define SET_BIT(x) 1 << x
