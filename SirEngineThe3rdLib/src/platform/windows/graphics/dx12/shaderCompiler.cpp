#include "platform/windows/graphics/dx12/shaderCompiler.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

// NOTE to work, dxcapi needs to happen after windows.h
#include <Windows.h>
//#include <atlbase.h>
#include <d3dcompiler.h>
#include <dxcapi.h>

#include "SirEngine/graphics/graphicsDefines.h"
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

  DxcCreateInstance(CLSID_DxcValidator, __uuidof(IDxcValidator),
                    (void **)&pValidator);

  // create a standard include handle, that resolves files using reltive path
  // to the file being compiled
  pLibrary->CreateIncludeHandler(&includeHandle);
}

DXCShaderCompiler::~DXCShaderCompiler() {
  pCompiler->Release();
  pLibrary->Release();
  includeHandle->Release();
}

ShaderCompileResult DXCShaderCompiler::compileShader(const char *shaderPath,
                                                     ShaderArgs &shaderArgs) {
  // creating a blob of data with the content of the shader
  const wchar_t *wshader = frameConvertWide(shaderPath);
  IDxcOperationResult *pResult;

  uint32_t fileSize;
  const char *program = persistentFileLoad(shaderPath, fileSize);
  if (program == nullptr) {
    return {nullptr, nullptr};
  }

  IDxcBlobEncoding *pSource;
  DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary),
                    reinterpret_cast<void **>(&pLibrary));
  pLibrary->CreateBlobWithEncodingFromPinned(
      // TODO should we return the null limiter in the size?
      // here we remove one from the file size since we include the null
      // terminator
      program, static_cast<uint32_t>(fileSize - 1), CP_UTF8, &pSource);

  // lets build compiler flags, for now they are simple so we can just switch
  // between two sets based on debug or not
  LPCWSTR *flags =
      shaderArgs.debug ? COMPILATION_FLAGS_DEBUG : COMPILATION_FLAGS;
  int flagsCount = shaderArgs.debug ? _countof(COMPILATION_FLAGS_DEBUG)
                                    : _countof(COMPILATION_FLAGS);

  // making a copy of the args vector

  auto &finalFlags = shaderArgs.splitCompilerArgsPointers;
  // adding the needed pointers to the list
  for (int i = 0; i < flagsCount; ++i) {
    // this flags are static no need to internalize them
    finalFlags.pushBack(const_cast<wchar_t *>(flags[i]));
  }
  flagsCount = static_cast<int>(shaderArgs.splitCompilerArgsPointers.size());

  // kick the compilation
  pCompiler->Compile(
      pSource,                // program text
      wshader,                // file name, mostly for error messages
      shaderArgs.entryPoint,  // entry point function
      shaderArgs.type,        // target profile
      const_cast<LPCWSTR *>(finalFlags.data()),  // compilation arguments
      flagsCount,     // number of compilation arguments
      nullptr, 0,     // name/value defines and their count
      includeHandle,  // handler for #include directives
      &pResult);

  // checking whether or not compilation was successful
  HRESULT hrCompilation;
  pResult->GetStatus(&hrCompilation);

  IDxcBlobEncoding *pPrintBlob;
  pResult->GetErrorBuffer(&pPrintBlob);


  size_t printBlobSize = pPrintBlob->GetBufferSize();
  const char *outLog = nullptr;

  if (printBlobSize != 0) {
    outLog = frameConcatenation(
        static_cast<char *>(pPrintBlob->GetBufferPointer()), "\n");
  }

  if (FAILED(hrCompilation)) {
    SE_CORE_ERROR("ERROR_LOG:\n {0}", outLog);
    pPrintBlob->Release();
    return {nullptr, outLog};
  }

  if (printBlobSize != 0) {
    SE_CORE_WARN("COMPILER LOG:\n {0}", outLog);
  }

  // extract compiled blob of data
  IDxcBlob *pResultBlob;
  pResult->GetResult(&pResultBlob);

  /*
  //NOTE Tried to get validation to work but does not seem to do much unluckily
  // I must be doing something wrong, since I had a shader that on shader playground would not
  //pass validation
  CComPtr<IDxcOperationResult> pValResult;
  HRESULT validationResult = pValidator->Validate(pResultBlob,DxcValidatorFlags_InPlaceEdit,&pValResult);
  assert(SUCCEEDED(validationResult));

  IDxcBlobEncoding *pPrintBlobValidation;
  pValResult->GetErrorBuffer(&pPrintBlobValidation);
  IDxcBlob *pResultBlob2;
  pValResult->GetResult(&pResultBlob2);
  HRESULT rr;
  pValResult->GetStatus(&rr);

  size_t printBlobSizeValidation = pPrintBlobValidation->GetBufferSize();
  if (printBlobSizeValidation) {
    outLog = frameConcatenation(
        static_cast<char *>(pPrintBlobValidation->GetBufferPointer()), "\n");
      
    SE_CORE_ERROR("ERROR_LOG:\n {0}", outLog);
    return {nullptr, outLog};
  }
  */

  // here we just copy to a regular blob so we dont have to leak out the IDXC
  // interface
  ID3DBlob *blob;
  const HRESULT hr = D3DCreateBlob(pResultBlob->GetBufferSize(), &blob);
  assert(SUCCEEDED(hr) && "could not create shader blob");
  memcpy(blob->GetBufferPointer(), pResultBlob->GetBufferPointer(),
         pResultBlob->GetBufferSize());
  pResult->Release();
  int size = pResultBlob->GetBufferSize();
  pResult->Release();
  pResultBlob->Release();

  stringFree(program);

  return {blob, outLog};
}
unsigned int DXCShaderCompiler::getShaderFlags(const ShaderArgs &shaderArgs) {
  unsigned int flags = 0;
  flags |= (shaderArgs.debug ? SHADER_FLAGS::SHADER_DEBUG : 0);

  return flags;
}
}  // namespace SirEngine::dx12
