#include "SirEngine/core.h"
#include "SirEngine/memory/resizableVector.h"
#include <d3dcommon.h>
#include <string>

struct IDxcCompiler;
struct IDxcLibrary;
struct IDxcIncludeHandler;

namespace SirEngine {
namespace dx12 {

enum SHADER_FLAGS { DEBUG = 1 };

template class SIR_ENGINE_API ResizableVector<wchar_t *>;
struct SIR_ENGINE_API ShaderArgs {
  ShaderArgs()
      : entryPoint(nullptr), type(nullptr), compilerArgs(nullptr),
        splitCompilerArgsPointers(20){};
  bool debug = false;

  wchar_t *entryPoint;
  wchar_t *type;
  wchar_t *compilerArgs;
  // this vector instead will hold the point of the args
  ResizableVector<wchar_t *> splitCompilerArgsPointers;
};

class SIR_ENGINE_API DXCShaderCompiler {
public:
  DXCShaderCompiler();
  ~DXCShaderCompiler();
  ID3DBlob *compileShader(const char *shaderPath, ShaderArgs &shaderArgs,
                          std::string *log = nullptr);
  unsigned int getShaderFlags(const ShaderArgs &shaderArgs);

private:
  IDxcCompiler *pCompiler = nullptr;
  IDxcLibrary *pLibrary = nullptr;
  IDxcIncludeHandler *includeHandle = nullptr;
};

} // namespace dx12
} // namespace SirEngine
