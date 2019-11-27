#include "platform/windows/graphics/vk/vkShaderManager.h"
#include "platform/windows/graphics/vk/vkShaderCompiler.h"

#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/runtimeString.h"
#include <d3dcompiler.h>

namespace SirEngine::vk {

void VkShaderManager::cleanup() {
  // we need to de-allocate everything
  for (auto s : m_stringToShader) {
    // TODO check how to release this
    // s.second.shader->Release();
  }
  m_stringToShader.clear();
}

VkShaderMetadata *extractShaderMetadata(StackAllocator &alloc,
                                        const VkShaderMapperData *mapper,
                                        const void *startOfData) {
  // extract metadata
  // I know, this aint pretty
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
  if (m_stringToShader.find(name) == m_stringToShader.end()) {
    // TODO just use scrap memory for this instead of a heap alloc
    uint32_t fileSize;
    const char *data = frameFileLoad(path, fileSize);

    const auto mapper = getMapperData<VkShaderMapperData>(data);
    const void *shaderPointer = data + sizeof(BinaryFileHeader);

    // TODO check if I can make Spirv blob const void*;
    const SpirVBlob blob{const_cast<void *>(shaderPointer), mapper->shaderSizeInByte};
    const VkShaderModule shaderModule =
        SirEngine::vk::VkShaderCompiler::spirvToShaderModule(blob);

    VkShaderMetadata *metadata =
        extractShaderMetadata(m_metadataAllocator, mapper, shaderPointer);

    m_stringToShader[name] = VkShaderBlob{shaderModule, metadata};
  }
}

void VkShaderManager::recompileShader(const char *path, const char *offsetPath,
                                      std::string *log) {
  // first thing first we need to get the shader metadata
  auto found = m_stringToShader.find(path);
  if (found == m_stringToShader.end()) {
    assert(0 && "could not find shader you are asking to recompile");
  }

  assert(0 && "VK shader recompilation not yet supported");
  /*
  ShaderBlob &blob = found->second;
  ShaderMetadata *meta = blob.metadata;
  VkShaderArgs args;
  args.debug = meta->shaderFlags & SHADER_FLAGS::DEBUG;
  args.entryPoint = meta->entryPoint;
  args.type = meta->type;
  args.compilerArgs = meta->compilerArgs;
  std::string compilerArgs(frameConvert(meta->compilerArgs));

  // TODO this is quite a mess, so many conversion with Wstring back and
  // fort small allocations etc.
  splitCompilerArgs(compilerArgs, args.splitCompilerArgsPointers);

  std::string fullShaderPath(offsetPath);
  fullShaderPath += blob.metadata->shaderPath;

  ID3DBlob *compiledShader =
      m_compiler->compileShader(fullShaderPath.c_str(), args, log);
  if (compiledShader != nullptr) {
    auto exp_path = std::filesystem::path(path);
    std::string name = exp_path.stem().string();
    // release old shader
    blob.shader->Release();
    // assign new one
    blob.shader = compiledShader;
    m_stringToShader[name] = blob;

    // update log
    if (log != nullptr) {
      std::string &logRef = *log;
      logRef += "Successfully compiled shader: ";
      logRef += name;
      logRef += "\n";
    }
  }
  */
}

VkShaderManager::~VkShaderManager() { delete m_compiler; }

void VkShaderManager::init() {
  m_metadataAllocator.initialize(METADATA_STACK_SIZE);
  m_compiler = new VkShaderCompiler();
}

void VkShaderManager::loadShadersInFolder(const char *directory) {
  std::vector<std::string> paths;

  // lets look first for shader from our resource compiler
  listFilesInFolder(directory, paths, "shader");
  for (const auto &p : paths) {
    loadShaderBinaryFile(p.c_str());
  }
}

} // namespace SirEngine::vk
