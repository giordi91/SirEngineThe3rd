#pragma once
#include <d3d12.h>

#include <DirectXMath.h>
#include <unordered_map>

namespace temp{
namespace rendering {
enum class ShaderLayout {
  vertexColor = 0,
  basicMesh,
  fullMesh,
  vertexUV,
  positionOnly,
  deferredNull,
  dxrMesh,
  INVALID
};

struct LayoutHandle {
  D3D12_INPUT_ELEMENT_DESC *layout;
  int size;
};

class ShadersLayoutRegistry {

public:
  // ID3D12InputLayout *getLayout(ShaderLayout layout,
  //                             ID3D10Blob *shaderBlob) const;

  void cleanup();
  LayoutHandle getShaderLayoutFromName(const std::string name);

  ShadersLayoutRegistry(); // private constructor
  ShadersLayoutRegistry(ShadersLayoutRegistry const &) =
      delete; // private copy constructor
  ShadersLayoutRegistry &operator=(ShadersLayoutRegistry const &) =
      delete; // private assignemnt operator

  void generateLayouts();

private:
  std::unordered_map<ShaderLayout, LayoutHandle> m_registry;
  static const std::unordered_map<std::string, ShaderLayout>
      NAME_TO_SHADER_LAYOUT;
};

} // end namespace rendering
} // namespace dx12
