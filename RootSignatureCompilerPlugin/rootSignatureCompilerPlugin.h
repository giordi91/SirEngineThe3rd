#pragma once
#include "resourceCompilerLib/resourcePlugin.h"
#include "resourceCompilerLib/core.h"

extern "C"
{
	bool RC_PLUGIN pluginRegisterFunction(PluginRegistry *registry);
}
