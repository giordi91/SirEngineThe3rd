#include <d3dcommon.h>

#include <string>

#include "SirEngine/core.h"
#include "SirEngine/memory/resizableVector.h"

struct IDxcCompiler;
struct IDxcLibrary;
struct IDxcIncludeHandler;

namespace SirEngine {
template class SIR_ENGINE_API ResizableVector<wchar_t *>;
namespace dx12 {

struct SIR_ENGINE_API ShaderArgs {
  ShaderArgs()
      : entryPoint(nullptr),
        type(nullptr),
        compilerArgs(nullptr),
        splitCompilerArgsPointers(20){};
  bool debug = false;

  wchar_t *entryPoint;
  wchar_t *type;
  wchar_t *compilerArgs;
  // this vector instead will hold the point of the args
  ResizableVector<wchar_t *> splitCompilerArgsPointers;
};

struct SIR_ENGINE_API ShaderCompileResult {
  ID3D10Blob *blob;
  const char *logResult;
};

class SIR_ENGINE_API DXCShaderCompiler {
 public:
  DXCShaderCompiler();
  ~DXCShaderCompiler();
  ShaderCompileResult compileShader(const char *shaderPath,
                                    ShaderArgs &shaderArgs);
  unsigned int getShaderFlags(const ShaderArgs &shaderArgs);

 private:
  IDxcCompiler *pCompiler = nullptr;
  IDxcLibrary *pLibrary = nullptr;
  IDxcIncludeHandler *includeHandle = nullptr;
};

}  // namespace dx12
}  // namespace SirEngine
