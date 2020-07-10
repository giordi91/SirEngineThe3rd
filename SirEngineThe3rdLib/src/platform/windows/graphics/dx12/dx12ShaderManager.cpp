#include "platform/windows/graphics/dx12/dx12ShaderManager.h"

#include <d3dcompiler.h>

#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/dx12/shaderCompiler.h"

namespace SirEngine {
namespace dx12 {

void Dx12ShaderManager::cleanup() {
  // we need to de-allocate everything
  for (auto s : m_stringToShader) {
    s.second.shader->Release();
  }
  m_stringToShader.clear();
}

ID3DBlob *loadCompiledShader(const std::string &filename) {
  std::ifstream fin(filename, std::ios::binary);
  assert(!fin.fail());
  fin.seekg(0, std::ios_base::end);
  std::ifstream::pos_type size = (int)fin.tellg();
  fin.seekg(0, std::ios_base::beg);

  ID3DBlob *blob;
  HRESULT hr = D3DCreateBlob(size, &blob);
  assert(SUCCEEDED(hr));

  fin.read(static_cast<char *>(blob->GetBufferPointer()), size);
  fin.close();
  return blob;
}

void Dx12ShaderManager::loadShaderFile(const char *path) {
  auto expPath = std::filesystem::path(path);
  std::string name = expPath.stem().string();
  if (m_stringToShader.find(name) == m_stringToShader.end()) {
    ID3DBlob *blob = loadCompiledShader(path);
    m_stringToShader[name].shader = blob;
    m_shaderNames.push_back(name);
  }
}

ShaderMetadata *extractShaderMetadata(StackAllocator &alloc,
                                      const ShaderMapperData *mapper,
                                      void *startOfData) {
  // extract metadata
  // I know, this aint pretty
  const wchar_t *typeW =
      (wchar_t *)((char *)(startOfData) + mapper->shaderSizeInByte);
  const wchar_t *entryW = (wchar_t *)((char *)(typeW) + mapper->typeSizeInByte);
  const char *shaderPath = (char *)(entryW) + mapper->entryPointInByte;
  const char *compilerArgs =
      mapper->compilerArgsInByte == 0
          ? nullptr
          : (char *)(shaderPath) + mapper->pathSizeInByte;

  // allocate the metadata
  auto *metadata = reinterpret_cast<ShaderMetadata *>(
      alloc.allocate(sizeof(ShaderMetadata)));
  // now that we have the metadata we can allocate enough memory for
  // the data and copy it over
  metadata->type =
      reinterpret_cast<wchar_t *>(alloc.allocate(mapper->typeSizeInByte));
  metadata->entryPoint =
      reinterpret_cast<wchar_t *>(alloc.allocate(mapper->entryPointInByte));
  metadata->shaderPath =
      reinterpret_cast<char *>(alloc.allocate(mapper->pathSizeInByte));

  metadata->compilerArgs = nullptr;
  if (compilerArgs != nullptr) {
    metadata->compilerArgs =
        reinterpret_cast<wchar_t *>(alloc.allocate(mapper->compilerArgsInByte));
  }

  // lets copy the data now
  memcpy(metadata->type, typeW, mapper->typeSizeInByte);
  memcpy(metadata->entryPoint, entryW, mapper->entryPointInByte);
  memcpy(metadata->shaderPath, shaderPath, mapper->pathSizeInByte);

  // TODO this is always true due to the fact that we get an empty string
  // might want to clean up
  if (compilerArgs != nullptr) {
    memcpy(metadata->compilerArgs, compilerArgs, mapper->compilerArgsInByte);
  }

  // only thing left is to extract the shader flags
  metadata->shaderFlags = mapper->shaderFlags;

  return metadata;
}

void Dx12ShaderManager::loadShaderBinaryFile(const char *path) {
  const auto expPath = std::filesystem::path(path);
  const std::string name = expPath.stem().string();
  if (m_stringToShader.find(name) == m_stringToShader.end()) {
    // TODO just use scrap memory for this instead of a heap alloc
    std::vector<char> data;
    readAllBytes(path, data);

    const auto mapper = getMapperData<ShaderMapperData>(data.data());
    void *shaderPointer = data.data() + sizeof(BinaryFileHeader);
    ID3DBlob *blob;
    const HRESULT hr = D3DCreateBlob(mapper->shaderSizeInByte, &blob);
    assert(SUCCEEDED(hr) && "could not create shader blob");
    memcpy(blob->GetBufferPointer(), shaderPointer, blob->GetBufferSize());

    ShaderMetadata *metadata =
        extractShaderMetadata(m_metadataAllocator, mapper, shaderPointer);

    m_stringToShader[name] = ShaderBlob{blob, metadata};
    m_shaderNames.push_back(name);
  }
}

void Dx12ShaderManager::recompileShader(const char *path,
                                        const char *offsetPath,
                                        std::string *log) {
  // first thing first we need to get the shader metadata
  auto found = m_stringToShader.find(path);
  if (found == m_stringToShader.end()) {
    assert(0 && "could not find shader you are asking to recompile");
  }

  ShaderBlob &blob = found->second;
  ShaderMetadata *meta = blob.metadata;
  ShaderArgs args;
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
    // here we recompile and override and existing one no need to update the
    // list of names
    m_stringToShader[name] = blob;

    // update log
    if (log != nullptr) {
      std::string &logRef = *log;
      logRef += "Successfully compiled shader: ";
      logRef += name;
      logRef += "\n";
    }
  }
}

Dx12ShaderManager::~Dx12ShaderManager() { delete m_compiler; }

void Dx12ShaderManager::initialize() {
  m_metadataAllocator.initialize(METADATA_STACK_SIZE);
  m_compiler = new DXCShaderCompiler();
}

void Dx12ShaderManager::loadShadersInFolder(const char *directory) {
  std::vector<std::string> paths;

  // lets look first for shader from our resource compiler
  listFilesInFolder(directory, paths, "shader");
  for (const auto &p : paths) {
    loadShaderBinaryFile(p.c_str());
  }

  // lets look for normally compiled shaders from visual studio
  paths.clear();
  listFilesInFolder(directory, paths, "cso");
  for (const auto &p : paths) {
    loadShaderFile(p.c_str());
  }
}

}  // namespace dx12
}  // namespace SirEngine
