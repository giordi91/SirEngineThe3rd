#include "platform/windows/graphics/dx12/textureManager.h"

#include "SirEngine/fileUtils.h"
#include <DXTK12/DDSTextureLoader.h>

namespace SirEngine {
namespace dx12 {
TextureHandle TextureManager::loadTexture(const char *path, bool dynamic) {
  bool res = fileExists(path);

  const std::string name = getFileName(path);
  assert(m_nameToHandle.find(name) == m_nameToHandle.end());

  assert(res);

  if (dynamic) {
    assert(0 && "dynamic textures are not yet implemented");
  }

  TextureData data;
  const std::string paths(path);
  const std::wstring pathws(paths.begin(), paths.end());
  std::unique_ptr<uint8_t[]> ddsData;
  std::vector<D3D12_SUBRESOURCE_DATA> subresources;

  batch.Begin();
  DirectX::CreateDDSTextureFromFile(dx12::DEVICE, batch, pathws.c_str(),
                                    &data.resource, false);
  batch.End(dx12::GLOBAL_COMMAND_QUEUE);

  // data is now loaded need to create handle etc
  TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) |
                       static_cast<uint32_t>(m_staticStorage.size())};

  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.isGamma = false;
  data.isHDR = false;
  data.format = data.resource->GetDesc().Format;
  m_staticStorage.push_back(data);

  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;
  return handle;
}

} // namespace dx12
} // namespace SirEngine
