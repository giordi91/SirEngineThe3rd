#pragma once
#include "ResourceCompilerLib/resourceCompiler/resourcePlugin.h"
#include "ResourceCompilerLib/resourceCompiler/core.h"

bool processModel(const std::string& assetPath, const std::string&outputPath);
extern "C"
{
	bool RC_PLUGIN pluginRegisterFunction(PluginRegistry *registry);
}
