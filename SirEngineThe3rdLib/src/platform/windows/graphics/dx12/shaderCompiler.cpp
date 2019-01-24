#include "platform/windows/graphics/dx12/shaderCompiler.h"
#include <cassert>
#include <d3dcompiler.h>
#include <fstream>
#include <iostream>

namespace temp{
namespace rendering {
ID3DBlob *compileShader(const std::wstring &filename,
                        const D3D_SHADER_MACRO *defines,
                        const std::string &entrypoint,
                        const std::string &target) {
  UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
  compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  HRESULT hr = S_OK;

  ID3DBlob *byteCode = nullptr;
  ID3DBlob *errors;
  hr = D3DCompileFromFile(filename.c_str(), defines,
                          D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(),
                          target.c_str(), compileFlags, 0, &byteCode, &errors);

  if (errors != nullptr)
  {
	  OutputDebugStringA((char *)errors->GetBufferPointer());
	  delete errors;
  }

  if (FAILED(hr)) {
    std::cout << "[SHADER]: error compiling shader" << std::endl;
  }

  return byteCode;
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


} // namespace rendering
} // namespace dx12
