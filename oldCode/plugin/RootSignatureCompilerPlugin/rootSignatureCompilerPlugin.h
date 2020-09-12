#pragma once
#include "resourceCompilerLib/core.h"
#include "resourceCompilerLib/resourcePlugin.h"

extern "C" {
bool RC_PLUGIN pluginRegisterFunction(PluginRegistry *registry);
}
