#include "SirEngine/core.h"
#include <d3dcommon.h>
#include <string>

struct IDxcCompiler;
struct IDxcLibrary;
struct IDxcIncludeHandler;

namespace SirEngine {
namespace dx12 {

enum SHADER_FLAGS { DEBUG = 1, AMD_INSTRINSICS = 2, NVIDIA_INSTRINSICS = 4 };

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
  ID3DBlob *compilerShader(const std::string &shaderPath,
                           const ShaderArgs &shaderArgs,
                           std::string *log = nullptr);
  unsigned int getShaderFlags(const ShaderArgs &shaderArgs);

private:
  IDxcCompiler *pCompiler = nullptr;
  IDxcLibrary *pLibrary = nullptr;
  IDxcIncludeHandler *includeHandle = nullptr;
};

} // namespace dx12
} // namespace SirEngine
