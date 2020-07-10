#include "platform/windows/graphics/vk/vkShaderManager.h"

#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/vk/vkShaderCompiler.h"
#include "vk.h"

namespace SirEngine::vk {

void VkShaderManager::cleanup() {
  int size = m_stringToShader.binCount();
  for (int i = 0; i < size; ++i) {
    bool used = m_stringToShader.isBinUsed(i);
    if (used) {
      VkShaderBlob v = m_stringToShader.getValueAtBin(i);
      vkDestroyShaderModule(vk::LOGICAL_DEVICE, v.shader, nullptr);
    }
  }
}

VkShaderMetadata *extractShaderMetadata(StackAllocator &alloc,
                                        const VkShaderMapperData *mapper,
                                        const void *startOfData) {
  // extract metadata
  // I know, this is not pretty
  SHADER_TYPE type = static_cast<SHADER_TYPE>(mapper->type);
  const char *entry = (char *)(startOfData) + mapper->shaderSizeInByte;
  const char *shaderPath = (char *)(entry) + mapper->entryPointInByte;
  const char *compilerArgs =
      mapper->compilerArgsInByte == 0
          ? nullptr
          : (char *)(shaderPath) + mapper->pathSizeInByte;

  // allocate the metadata
  auto *metadata = reinterpret_cast<VkShaderMetadata *>(
      alloc.allocate(sizeof(VkShaderMetadata)));
  // now that we have the metadata we can allocate enough memory for
  // the data and copy it over
  metadata->type = type;
  metadata->entryPoint =
      reinterpret_cast<char *>(alloc.allocate(mapper->entryPointInByte));
  metadata->shaderPath =
      reinterpret_cast<char *>(alloc.allocate(mapper->pathSizeInByte));

  // lets copy the data now
  memcpy(metadata->entryPoint, entry, mapper->entryPointInByte);
  memcpy(metadata->shaderPath, shaderPath, mapper->pathSizeInByte);

  // only thing left is to extract the shader flags
  metadata->shaderFlags = mapper->shaderFlags;

  return metadata;
}

void VkShaderManager::loadShaderBinaryFile(const char *path) {
  const std::string name = getFileName(path);

  if (!m_stringToShader.containsKey(name.c_str())) {
    // TODO just use scrap memory for this instead of a heap alloc
    uint32_t fileSize;
    const char *data = frameFileLoad(path, fileSize);

    const auto mapper = getMapperData<VkShaderMapperData>(data);
    const void *shaderPointer = data + sizeof(BinaryFileHeader);

    // TODO check if I can make Spirv blob const void*;
    const SpirVBlob blob{const_cast<void *>(shaderPointer),
                         mapper->shaderSizeInByte};
    const VkShaderModule shaderModule =
        SirEngine::vk::VkShaderCompiler::spirvToShaderModule(blob);

    VkShaderMetadata *metadata =
        extractShaderMetadata(m_metadataAllocator, mapper, shaderPointer);

    const char *cname = globals::STRING_POOL->allocatePersistent(name.c_str());
    m_stringToShader.insert(cname, VkShaderBlob{shaderModule, metadata});
    m_shaderNames.pushBack(cname);
  }
}

const char *VkShaderManager::recompileShader(const char *path,
                                             const char *offsetPath) {
  // first thing first we need to get the shader metadata

  VkShaderBlob blob{};
  bool result = m_stringToShader.get(path, blob);
  if (!result) {
    assert(0 && "could not find shader you are asking to recompile");
  }

  VkShaderMetadata *meta = blob.metadata;
  VkShaderArgs args;
  args.debug = true;
  args.type = meta->type;

  SE_CORE_INFO("requested shader recompilation for {0}\n {1}", path,
               offsetPath);
  std::string log;
  // we need the shader full path
  std::string fullShaderPath(offsetPath);
  fullShaderPath += blob.metadata->shaderPath;

  VkShaderModule module =
      m_compiler->compileToShaderModule(fullShaderPath.c_str(), args, &log);
  if (module != nullptr) {
    auto expPath = std::filesystem::path(path);
    std::string name = expPath.stem().string();
    // release old shader
    vkDestroyShaderModule(vk::LOGICAL_DEVICE, blob.shader, nullptr);
    // assign new one
    blob.shader = module;
    m_stringToShader.insert(name.c_str(), blob);

    // prepare output log
    const char *out;
    if (!log.empty()) {
      out = globals::STRING_POOL->concatenateFrame(
          log.c_str(), "Successfully compiled shader: ");
      out = globals::STRING_POOL->concatenateFrame(out, "\n", name.c_str());
      //      logRef += compileResult.logResult;
    } else {
      out = globals::STRING_POOL->concatenateFrame(
          "Successfully compiled shader: ", "\n", name.c_str());
    }
    return out;
  }
  return nullptr;
}

VkShaderManager::~VkShaderManager() { delete m_compiler; }

void VkShaderManager::initialize() {
  m_metadataAllocator.initialize(METADATA_STACK_SIZE);
  m_compiler = new VkShaderCompiler();
}

void VkShaderManager::loadShaderFile(const char *path) { assert(0); }

void VkShaderManager::loadShadersInFolder(const char *directory) {
  std::vector<std::string> paths;

  // lets look first for shader from our resource compiler
  listFilesInFolder(directory, paths, "shader");
  for (const auto &p : paths) {
    loadShaderBinaryFile(p.c_str());
  }
}

}  // namespace SirEngine::vk
