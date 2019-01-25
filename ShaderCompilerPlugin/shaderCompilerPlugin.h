#pragma once
#include "resourceCompilerLib/resourcePlugin.h"
#include "resourceCompilerLib/core.h"

bool processShader(const std::string& assetPath, const std::string&outputPath);

extern "C"
{
	bool RC_PLUGIN pluginRegisterFunction(PluginRegistry *registry);
}