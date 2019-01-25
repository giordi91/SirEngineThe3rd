#pragma once
#include <d3d12.h>
#include <vector>
#include <string>
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

struct ResultRoot {
  std::string name;
  ID3DBlob *blob;
  temp::rendering::ROOT_FILE_TYPE type;
};

void processSignatureFile(const char *path, std::vector<ResultRoot> &blobs);
