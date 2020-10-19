#pragma once
#include <string>

extern "C" {
bool processPoints(const std::string &assetPath, const std::string &outputPath,
                   const std::string &);
}
