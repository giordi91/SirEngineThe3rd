#include "platform/windows/graphics/dx12/textureManager.h"

#include "SirEngine/fileUtils.h"
#include <DXTK12/DDSTextureLoader.h>

namespace SirEngine {
namespace dx12 {

TextureManager::TextureData &TextureManager::getFreeTextureData(uint32_t& index) {
  // if there is not free index
  if (m_freeSlotIndex == 0) {
    m_staticStorage.emplace_back(TextureData{});
	index = static_cast<uint32_t>(m_staticStorage.size() - 1);
    return m_staticStorage[m_staticStorage.size() - 1];
  } else {
    // lets re-use the slot
    TextureData &data = m_staticStorage[m_freeSlots[0]];
	index = m_freeSlots[0];
    // patch the hole
    m_freeSlots[0] = m_freeSlots[m_freeSlotIndex-1];
	--m_freeSlotIndex;
    return data;
  }
}

TextureHandle TextureManager::loadTexture(const char *path, bool dynamic) {
  bool res = fileExists(path);

  const std::string name = getFileName(path);
  assert(m_nameToHandle.find(name) == m_nameToHandle.end());

  assert(res);

  if (dynamic) {
    assert(0 && "dynamic textures are not yet implemented");
  }

  uint32_t index;
  TextureData &data = getFreeTextureData(index);
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
                       index};

  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = D3D12_RESOURCE_STATE_COMMON;
  // m_staticStorage.push_back(data);

  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;
  return handle;
}

TextureHandle TextureManager::initializeFromResource(
    ID3D12Resource *resource, const char *name, D3D12_RESOURCE_STATES state) {
  // since we are passing one resource, by definition the resource is static
  // data is now loaded need to create handle etc
  uint32_t index;
  TextureData &data = getFreeTextureData(index);
  TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) |
                       index};

  data.resource = resource;
  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = state;
  // m_staticStorage.push_back(data);

  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;
  SE_CORE_INFO(m_staticStorage.size());
  return handle;
}
} // namespace dx12
} // namespace SirEngine
