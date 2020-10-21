#include <d3dcommon.h>


#include "SirEngine/core.h"
#include "SirEngine/memory/cpu/resizableVector.h"

struct IDxcCompiler;
struct IDxcLibrary;
struct IDxcValidator;
struct IDxcIncludeHandler;

namespace SirEngine {
namespace dx12 {

struct ShaderArgs {
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

struct ShaderCompileResult {
  ID3D10Blob *blob;
  const char *logResult;
};

class DXCShaderCompiler {
 public:
  DXCShaderCompiler();
  ~DXCShaderCompiler();
  ShaderCompileResult compileShader(const char *shaderPath,
                                    ShaderArgs &shaderArgs);
  ShaderCompileResult toSpirv(const char *shaderPath,
                                    ShaderArgs &shaderArgs);
  unsigned int getShaderFlags(const ShaderArgs &shaderArgs);

 private:
  IDxcCompiler *pCompiler = nullptr;
  IDxcLibrary *pLibrary = nullptr;
  IDxcValidator *pValidator= nullptr;
  IDxcIncludeHandler *includeHandle = nullptr;
};

}  // namespace dx12
}  // namespace SirEngine
