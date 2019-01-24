#pragma once
#include <d3dcommon.h>
#include <string>

namespace temp{
namespace rendering {

ID3DBlob* compileShader(const std::wstring &filename,
                                               const D3D_SHADER_MACRO *defines,
                                               const std::string &entrypoint,
                                               const std::string &target);

ID3DBlob* loadCompiledShader(const std::string &filename);
} // namespace rendering
} // namespace dx12
