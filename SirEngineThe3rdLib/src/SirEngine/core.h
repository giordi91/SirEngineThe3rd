#pragma once

// defined here such that I don't have to worry about including cstdint
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

constexpr uint16_t UINT16_MAX_VALUE = 65535;
constexpr uint8_t UINT8_MAX_VALUE = 255;
constexpr int8_t INT8_MAX_VALUE = 127;

#define SET_BIT(x) 1 << x
