
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"

#include "SirEngine/io/fileUtils.h"
#include "rootSignatureCompile.h"

namespace SirEngine::dx12 {

void Dx12RootSignatureManager::cleanup() {
  // cleanup the allocated root signatures
  // for (const auto& it : m_rootRegister) {
  //    getRootSignatureFromHandle(it.second)->Release();
  //}
  // m_rootRegister.clear();
}

RSHandle Dx12RootSignatureManager::loadSignatureFromMeta(
    const char *path, graphics::MaterialMetadata *metadata) {
  RootCompilerResult result = processSignatureFile(path, metadata);
  const std::string name = getFileName(path);

  // generate the handle
  uint32_t index;
  RSData &rsdata = m_rsPool.getFreeMemoryData(index);
  rsdata.rs = result.root;
  const RSHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  rsdata.magicNumber = MAGIC_NUMBER_COUNTER;
  rsdata.isFlatRoot = true;
  rsdata.descriptorCount = result.descriptorCount;
  rsdata.bindingSlots[0] = result.bindingSlots[0];
  rsdata.bindingSlots[1] = result.bindingSlots[1];
  rsdata.bindingSlots[2] = result.bindingSlots[2];
  rsdata.bindingSlots[3] = result.bindingSlots[3];
  m_rootRegister.insert(name.c_str(), handle);
  ++MAGIC_NUMBER_COUNTER;
  return handle;
}

}  // namespace SirEngine::dx12
