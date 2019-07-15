#include "platform/windows/graphics/dx12/shaderCompiler.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

// NOTE to work, dxcapi needs to happen after windows.h
#include <windows.h>

#include <dxcapi.h>

#include <d3dcompiler.h>

namespace SirEngine {
namespace dx12 {

LPCWSTR COMPILATION_FLAGS_DEBUG[] = {L"/Zi", L"/Od"};
LPCWSTR COMPILATION_FLAGS[] = {L"/O3"};

DXCShaderCompiler::DXCShaderCompiler() {
  // lets create an instance of the compiler
  DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler),
                    (void **)&pCompiler);

  // allocating the library for utility functions
  DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary),
                    (void **)&pLibrary);

  // create a standard include handle, that resolves files using reltive path
  // to the file being compiled
  pLibrary->CreateIncludeHandler(&includeHandle);
}

DXCShaderCompiler::~DXCShaderCompiler() {
  pCompiler->Release();
  pLibrary->Release();
  includeHandle->Release();
}

ID3DBlob *DXCShaderCompiler::compileShader(const std::string &shaderPath,
                                           const ShaderArgs &shaderArgs,
                                           std::string *log) {
  // creating a blob of data with the content of the shader
  std::wstring wshader{shaderPath.begin(), shaderPath.end()};
  IDxcOperationResult *pResult;
  // loading shader into straem
  std::ifstream shaderStream(shaderPath);
  std::string shaderContent((std::istreambuf_iterator<char>(shaderStream)),
                            std::istreambuf_iterator<char>());
  const char *Program = shaderContent.c_str();
  IDxcBlobEncoding *pSource;
  DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary),
                    (void **)&pLibrary);
  pLibrary->CreateBlobWithEncodingFromPinned(
      Program, static_cast<uint32_t>(shaderContent.size()), CP_UTF8, &pSource);

  // lets build compiler flags, for now they are simple so we can just switch
  // between two sets based on debug or not
  LPCWSTR *flags =
      shaderArgs.debug ? COMPILATION_FLAGS_DEBUG : COMPILATION_FLAGS;
  int flagsCount = shaderArgs.debug ? _countof(COMPILATION_FLAGS_DEBUG)
                                    : _countof(COMPILATION_FLAGS);

  // making a copy of the args vector
  auto finalFlags = shaderArgs.splitCompilerArgsPointers;
  // adding the needed pointers to the list
  for (int i = 0; i < flagsCount; ++i) {
    auto ptr = flags[i];
    finalFlags.push_back((wchar_t *)ptr);
  }
  flagsCount += static_cast<int>(shaderArgs.splitCompilerArgsPointers.size());
  //std::wstring test = L"/D AMD";
  //finalFlags[0] = (wchar_t*)test.c_str();

  // kick the compilation
  pCompiler->Compile(pSource,          // program text
                     wshader.c_str(),  // file name, mostly for error messages
                     shaderArgs.entryPoint.c_str(),  // entry point function
                     shaderArgs.type.c_str(),        // target profile
                     (LPCWSTR*)finalFlags.data(),              // compilation arguments
                     flagsCount,     // number of compilation arguments
                     nullptr, 0,     // name/value defines and their count
                     includeHandle,  // handler for #include directives
                     &pResult);

  // checking whether or not compilation was successiful
  HRESULT hrCompilation;
  pResult->GetStatus(&hrCompilation);

  if (FAILED(hrCompilation)) {
    IDxcBlobEncoding *pPrintBlob;
    pResult->GetErrorBuffer(&pPrintBlob);

    const std::string errorOut(
        static_cast<char *>(pPrintBlob->GetBufferPointer()),
        pPrintBlob->GetBufferSize());
    SE_CORE_ERROR("ERROR_LOG:\n {0}", errorOut);
    if (log != nullptr) {
      (*log) += errorOut;
      (*log) += "\n";
    }
    pPrintBlob->Release();
    return nullptr;
  }

  // extract compiled blob of data
  IDxcBlob *pResultBlob;
  pResult->GetResult(&pResultBlob);

  // here we just copy to a regular blob so we dont have to leak out the IDXC
  // interface
  ID3DBlob *blob;
  HRESULT hr = D3DCreateBlob(pResultBlob->GetBufferSize(), &blob);
  assert(SUCCEEDED(hr) && "could not create shader blob");
  memcpy(blob->GetBufferPointer(), pResultBlob->GetBufferPointer(),
         pResultBlob->GetBufferSize());

  pResult->Release();
  pResultBlob->Release();

  return blob;
}
unsigned int DXCShaderCompiler::getShaderFlags(const ShaderArgs &shaderArgs) {
  unsigned int flags = 0;
  flags |= (shaderArgs.debug ? SHADER_FLAGS::DEBUG : 0);

  return flags;
}
}  // namespace dx12
}  // namespace SirEngine
