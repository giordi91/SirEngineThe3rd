#include "platform/windows/graphics/dx12/shaderCompiler.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

// NOTE to work, dxcapi needs to happen after windows.h
#include <Windows.h>

#include <dxcapi.h>

#include <d3dcompiler.h>
#include "SirEngine/runtimeString.h"

namespace SirEngine::dx12 {

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

ID3DBlob *DXCShaderCompiler::compileShader(const char *shaderPath,
                                           const ShaderArgs &shaderArgs,
                                           std::string *log) {

  // creating a blob of data with the content of the shader
  const wchar_t *wshader = frameConvertWide(shaderPath);
  IDxcOperationResult *pResult;

  uint32_t fileSize;
  const char *program = persistentFileLoad(shaderPath, fileSize);

  IDxcBlobEncoding *pSource;
  DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary),
                    reinterpret_cast<void **>(&pLibrary));
  pLibrary->CreateBlobWithEncodingFromPinned(
      program, static_cast<uint32_t>(fileSize), CP_UTF8, &pSource);

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
    finalFlags.push_back(const_cast<wchar_t *>(flags[i]));
  }
  flagsCount += static_cast<int>(shaderArgs.splitCompilerArgsPointers.size());

  // kick the compilation
  pCompiler->Compile(pSource,  // program text
                     wshader,  // file name, mostly for error messages
                     shaderArgs.entryPoint.c_str(),  // entry point function
                     shaderArgs.type.c_str(),        // target profile
                     const_cast<LPCWSTR *>(finalFlags.data()),   // compilation arguments
                     flagsCount,     // number of compilation arguments
                     nullptr, 0,     // name/value defines and their count
                     includeHandle,  // handler for #include directives
                     &pResult);

  // checking whether or not compilation was successful
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
  const HRESULT hr = D3DCreateBlob(pResultBlob->GetBufferSize(), &blob);
  assert(SUCCEEDED(hr) && "could not create shader blob");
  memcpy(blob->GetBufferPointer(), pResultBlob->GetBufferPointer(),
         pResultBlob->GetBufferSize());

  pResult->Release();
  pResultBlob->Release();

  stringFree(program);

  return blob;
}
unsigned int DXCShaderCompiler::getShaderFlags(const ShaderArgs &shaderArgs) {
  unsigned int flags = 0;
  flags |= (shaderArgs.debug ? SHADER_FLAGS::DEBUG : 0);

  return flags;
}
}  // namespace SirEngine::dx12
