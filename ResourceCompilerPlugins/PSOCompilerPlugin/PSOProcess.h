#pragma once
#include <d3d12.h>
#include "platform/windows/graphics/dx12/PSOManager.h"

struct PSOResult {
  ID3DBlob *blob;
  SirEngine::dx12::PSOType type;
};

void processPSO(const char *path, PSOResult &blob);
