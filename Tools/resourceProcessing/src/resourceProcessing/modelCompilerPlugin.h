#pragma once
#include "resourceCompilerLib/core.h"
#include "resourceCompilerLib/resourcePlugin.h"

extern "C" {
bool processModel(const std::string &assetPath, const std::string &outputPath,
                  const std::string &args);
}
