#include "platform/windows/graphics/dx12/shaderManager.h"

#include "platform/windows/graphics/dx12/shaderCompiler.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include <d3dcompiler.h>

namespace SirEngine {
namespace dx12 {

void ShaderManager::cleanup() {
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

void ShaderManager::loadShaderFile(const char *path) {

  auto exp_path = std::experimental::filesystem::path(path);
  std::string name = exp_path.stem().string();
  if (m_stringToShader.find(name) == m_stringToShader.end()) {
    ID3DBlob *blob = loadCompiledShader(path);
    m_stringToShader[name].shader = blob;
  }
}

ShaderMetadata *extractShaderMetadata(StackAllocator &alloc,
                                      ShaderMapperData *mapper,
                                      void *startOfData) {

  // extract metadata
  wchar_t *typeW =
      (wchar_t *)((char *)(startOfData) + mapper->shaderSizeInBtye);
  wchar_t *entryW = (wchar_t *)((char *)(typeW) + mapper->typeSizeInByte);
  wchar_t *shaderPath =
      (wchar_t *)((char *)(entryW) + mapper->entryPointInByte);

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
      reinterpret_cast<char *>(alloc.allocate(mapper->pathSizeInBtype));

  // lets copy the data now
  memcpy(metadata->type, typeW, mapper->typeSizeInByte);
  memcpy(metadata->entryPoint, entryW, mapper->entryPointInByte);
  memcpy(metadata->shaderPath, shaderPath, mapper->pathSizeInBtype);

  // only thing left is to extract the shader flags
  metadata->shaderFlags = mapper->shaderFlags;

  return metadata;
}

void ShaderManager::loadShaderBinaryFile(const char *path) {

  auto exp_path = std::experimental::filesystem::path(path);
  std::string name = exp_path.stem().string();
  if (m_stringToShader.find(name) == m_stringToShader.end()) {

    // TODO just use scrap memory for this instead of a heap alloc
    std::vector<char> data;
    readAllBytes(path, data);

    auto mapper = getMapperData<ShaderMapperData>(data.data());
    void *shaderPointer = data.data() + sizeof(BinaryFileHeader);
    ID3DBlob *blob;
    HRESULT hr = D3DCreateBlob(mapper->shaderSizeInBtye, &blob);
    assert(SUCCEEDED(hr) && "could not create shader blob");
    memcpy(blob->GetBufferPointer(), shaderPointer, blob->GetBufferSize());

    ShaderMetadata *metadata =
        extractShaderMetadata(m_metadataAllocator, mapper, shaderPointer);

    m_stringToShader[name] = ShaderBlob{blob, metadata};
  }
}

void ShaderManager::recompileShader(const char *path, std::string *log) {

  // first thing first we need to get the shader metadata
  auto found = m_stringToShader.find(path);
  if (found == m_stringToShader.end()) {
    assert(0 && "could not find shader you are asking to recompile");
  }

  ShaderBlob &blob = found->second;
  ShaderMetadata *meta = blob.metadata;
  ShaderArgs args;
  args.debug = meta->shaderFlags & SHADER_FLAGS::DEBUG;
  // TODO am I going to support instrinsics? SM6 supports most of the
  // instruction I wanted
  args.isAMD = 0;
  args.isNVidia = 0;
  args.entryPoint = meta->entryPoint;
  args.type = meta->type;

  ID3DBlob *compiledShader =
      m_compiler->compilerShader(blob.metadata->shaderPath, args, log);
  if (compiledShader != nullptr) {
    auto exp_path = std::experimental::filesystem::path(path);
    std::string name = exp_path.stem().string();
	//release old shader
	blob.shader->Release();
	//assign new one
	blob.shader = compiledShader;
    m_stringToShader[name] = blob;

	//update log
	(*log)+= "Successifully compiled shader: ";
	(*log)+= name;
	(*log)+= "\n";
  }
}

ShaderManager::~ShaderManager() { delete m_compiler; }

void ShaderManager::init() {
  m_metadataAllocator.initialize(METADATA_STACK_SIZE);
  m_compiler = new DXCShaderCompiler();
}

void ShaderManager::loadShadersInFolder(const char *directory) {
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

} // namespace dx12
} // namespace SirEngine
