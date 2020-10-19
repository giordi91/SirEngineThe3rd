#pragma once
#include <string>

extern "C" {
bool processAnim(const std::string &assetPath, const std::string &outputPath,
                 const std::string &args);
}
