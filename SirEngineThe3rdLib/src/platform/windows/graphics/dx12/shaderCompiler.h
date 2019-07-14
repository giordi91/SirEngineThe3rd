#include "SirEngine/core.h"
#include <d3dcommon.h>
#include <string>
#include <vector>

struct IDxcCompiler;
struct IDxcLibrary;
struct IDxcIncludeHandler;

namespace SirEngine {
namespace dx12 {

enum SHADER_FLAGS { DEBUG = 1};

struct SIR_ENGINE_API ShaderArgs {
  bool debug = false;

  std::wstring entryPoint;
  std::wstring type;
  std::wstring compilerArgs; 
  //I Know aint pretty, but for the time being will do, when I will have proper
  //string pools /allocators I will fix this
  //this vector holds the compiler args but split into each one separately
  std::vector<std::wstring>splitCompilerArgs;
  //this vector instead will hold the point of the args
  std::vector<wchar_t*>splitCompilerArgsPointers;
};

class SIR_ENGINE_API DXCShaderCompiler {
public:
  DXCShaderCompiler();
  ~DXCShaderCompiler();
  ID3DBlob *compileShader(const std::string &shaderPath,
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
