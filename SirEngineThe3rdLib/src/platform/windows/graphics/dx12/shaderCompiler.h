#include "SirEngine/core.h"
#include <string>
#include <d3dcommon.h>

struct IDxcCompiler;
struct IDxcLibrary;
struct IDxcIncludeHandler;

namespace SirEngine {
namespace dx12 {

struct SIR_ENGINE_API ShaderArgs {
  bool debug = false;
  bool isAMD = false;
  bool isNVidia = false;

  std::wstring entryPoint;
  std::wstring type;
};

class SIR_ENGINE_API DXCShaderCompiler {
public:
  DXCShaderCompiler();
  ~DXCShaderCompiler();
  ID3DBlob*  compilerShader(const std::string &shaderPath, const ShaderArgs& shaderArgs);


private:
  IDxcCompiler *pCompiler = nullptr;
  IDxcLibrary *pLibrary = nullptr;
  IDxcIncludeHandler *includeHandle = nullptr;
};

} // namespace dx12
} // namespace SirEngine
