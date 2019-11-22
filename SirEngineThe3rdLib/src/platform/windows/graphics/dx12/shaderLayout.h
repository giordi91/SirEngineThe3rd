#pragma once
#include <d3d12.h>
#include <unordered_map>
#include "SirEngine/core.h"

    namespace SirEngine {
  namespace dx12 {
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

  struct LayoutHandle final {
    D3D12_INPUT_ELEMENT_DESC *layout;
    int size;
  };

  class SIR_ENGINE_API ShadersLayoutRegistry final {

  public:
    // ID3D12InputLayout *getLayout(ShaderLayout layout,
    //                             ID3D10Blob *shaderBlob) const;

    void cleanup();
    LayoutHandle getShaderLayoutFromName(const std::string name);

    ShadersLayoutRegistry(); // private constructor
    ShadersLayoutRegistry(ShadersLayoutRegistry const &) =
        delete; // private copy constructor
    ShadersLayoutRegistry &operator=(ShadersLayoutRegistry const &) =
        delete; // private assignment operator

    void generateLayouts();

  private:
    std::unordered_map<ShaderLayout, LayoutHandle> m_registry;
  };

  } // namespace dx12
} // namespace dx12
