#pragma once
#include <d3d12.h>
#include "platform/windows/graphics/dx12/PSOManager.h"


SirEngine::dx12::PSOCompileResult processPSO(const char *path, const char *shaderPath);
