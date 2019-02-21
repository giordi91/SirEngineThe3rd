#include "platform/windows/graphics/dx12/shaderLayout.h"
#include <cassert>

namespace SirEngine {
namespace dx12 {

const std::unordered_map<std::string, ShaderLayout> NAME_TO_SHADER_LAYOUT = {
    {"vertexColor", ShaderLayout::vertexColor},
    {"basicMesh", ShaderLayout::basicMesh},
    {"fullMesh", ShaderLayout::fullMesh},
    {"vertexUV", ShaderLayout::vertexUV},
    {"dxrMesh", ShaderLayout::dxrMesh},
    {"positionOnly", ShaderLayout::positionOnly},
    {"deferredNull", ShaderLayout::deferredNull}};

ShadersLayoutRegistry::ShadersLayoutRegistry() { generateLayouts(); }

void ShadersLayoutRegistry::cleanup() {
  for (auto &v : m_registry) {
    delete[] v.second.layout;
  }
}

LayoutHandle
ShadersLayoutRegistry::getShaderLayoutFromName(const std::string name) {
  auto found = NAME_TO_SHADER_LAYOUT.find(name);
  if (found != NAME_TO_SHADER_LAYOUT.end()) {
    auto layoutFound = m_registry.find(found->second);
    if (layoutFound != m_registry.end()) {
      return layoutFound->second;
    }
    assert(0 && "could not find shader layout of given type");
  }
  assert(0 && "could not find shader layout");
  return LayoutHandle{nullptr, -1};
}

inline void zeroOutLayouts(LayoutHandle &handle) {
  for (int i = 0; i < handle.size; ++i) {
    ZeroMemory(handle.layout + i, sizeof(D3D12_INPUT_ELEMENT_DESC));
  }
}

inline void generatePositionFloat4(D3D12_INPUT_ELEMENT_DESC *ptr) {
  ptr->SemanticName = "POSITION";
  ptr->SemanticIndex = 0;
  ptr->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  ptr->InputSlot = 0;
  ptr->AlignedByteOffset = 0;
  ptr->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  ptr->InstanceDataStepRate = 0;
}

inline void generatePositionFloat3(D3D12_INPUT_ELEMENT_DESC *ptr) {
  ptr->SemanticName = "POSITION";
  ptr->SemanticIndex = 0;
  ptr->Format = DXGI_FORMAT_R32G32B32_FLOAT;
  ptr->InputSlot = 0;
  ptr->AlignedByteOffset = 0;
  ptr->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  ptr->InstanceDataStepRate = 0;
}

inline void generateNormalsFloat4(D3D12_INPUT_ELEMENT_DESC *ptr,
                                  unsigned int offset = 16) {
  ptr->SemanticName = "NORMAL";
  ptr->SemanticIndex = 0;
  ptr->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  ptr->InputSlot = 0;
  ptr->AlignedByteOffset = offset;
  ptr->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  ptr->InstanceDataStepRate = 0;
}
inline void generateNormalsFloat3(D3D12_INPUT_ELEMENT_DESC *ptr,
                                  unsigned int offset = 12) {
  ptr->SemanticName = "NORMAL";
  ptr->SemanticIndex = 0;
  ptr->Format = DXGI_FORMAT_R32G32B32_FLOAT;
  ptr->InputSlot = 0;
  ptr->AlignedByteOffset = offset;
  ptr->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  ptr->InstanceDataStepRate = 0;
}

inline void generateTangentsFloat4(D3D12_INPUT_ELEMENT_DESC *ptr,
                                   unsigned int offset = 32) {
  ptr->SemanticName = "TANGENTS";
  ptr->SemanticIndex = 0;
  ptr->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  ptr->InputSlot = 0;
  ptr->AlignedByteOffset = offset;
  ptr->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  ptr->InstanceDataStepRate = 0;
}

inline void generateUVsFloat4(D3D12_INPUT_ELEMENT_DESC *ptr,
                              unsigned int offset = 32) {
  ptr->SemanticName = "TEXCOORD";
  ptr->SemanticIndex = 0;
  ptr->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  ptr->InputSlot = 0;
  ptr->AlignedByteOffset = offset;
  ptr->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  ptr->InstanceDataStepRate = 0;
}
inline void generateUVsFloat2(D3D12_INPUT_ELEMENT_DESC *ptr,
                              unsigned int offset = 32) {
  ptr->SemanticName = "TEXCOORD";
  ptr->SemanticIndex = 0;
  ptr->Format = DXGI_FORMAT_R32G32_FLOAT;
  ptr->InputSlot = 0;
  ptr->AlignedByteOffset = offset;
  ptr->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  ptr->InstanceDataStepRate = 0;
}

inline void generateColorFloat4(D3D12_INPUT_ELEMENT_DESC *ptr,
                                unsigned int offset = 16) {
  ptr->SemanticName = "COLOR";
  ptr->SemanticIndex = 0;
  ptr->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  ptr->InputSlot = 0;
  ptr->AlignedByteOffset = offset;
  ptr->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  ptr->InstanceDataStepRate = 0;
}

void ShadersLayoutRegistry::generateLayouts() {
  // this is to generate the built in layouts, we can also register layouts
  // on the fly if needed, we just need to add the proper enum entry

  // Vertex Color
  LayoutHandle vertexColorHandle{new D3D12_INPUT_ELEMENT_DESC[2], 2};
  zeroOutLayouts(vertexColorHandle);

  generatePositionFloat3(vertexColorHandle.layout);
  generateColorFloat4(vertexColorHandle.layout + 1, 12);

  m_registry[ShaderLayout::vertexColor] = vertexColorHandle;

  // Vertex Color
  LayoutHandle dxrMeshHandle{new D3D12_INPUT_ELEMENT_DESC[2], 2};
  zeroOutLayouts(vertexColorHandle);

  generatePositionFloat4(dxrMeshHandle.layout);
  generateNormalsFloat4(dxrMeshHandle.layout + 1, 16);

  m_registry[ShaderLayout::dxrMesh] = dxrMeshHandle;

  // basic mesh layout
  LayoutHandle basicMeshHandle{new D3D12_INPUT_ELEMENT_DESC[3], 3};
  zeroOutLayouts(basicMeshHandle);
  generatePositionFloat4(basicMeshHandle.layout);
  generateNormalsFloat4(basicMeshHandle.layout + 1, 16);
  generateUVsFloat4(basicMeshHandle.layout + 2, 32);

  m_registry[ShaderLayout::basicMesh] = basicMeshHandle;

  // full mesh layout
  LayoutHandle fullMeshHandle{new D3D12_INPUT_ELEMENT_DESC[4], 4};
  zeroOutLayouts(fullMeshHandle);
  generatePositionFloat3(fullMeshHandle.layout);
  generateNormalsFloat3(fullMeshHandle.layout + 1, 12);
  generateUVsFloat2(fullMeshHandle.layout + 2, 24);
  generateTangentsFloat4(fullMeshHandle.layout + 3, 32);

  m_registry[ShaderLayout::fullMesh] = fullMeshHandle;

  // Vertex Instanced
  LayoutHandle vertexUVHandle{new D3D12_INPUT_ELEMENT_DESC[2], 2};
  zeroOutLayouts(vertexUVHandle);

  generatePositionFloat4(vertexUVHandle.layout);
  generateUVsFloat4(vertexUVHandle.layout + 1, 16);

  m_registry[ShaderLayout::vertexUV] = vertexUVHandle;

  // position only only

  LayoutHandle positionOnlyHandleHandle{new D3D12_INPUT_ELEMENT_DESC[1], 1};
  zeroOutLayouts(positionOnlyHandleHandle);

  generatePositionFloat4(positionOnlyHandleHandle.layout);
  m_registry[ShaderLayout::positionOnly] = positionOnlyHandleHandle;

  m_registry[ShaderLayout::deferredNull] = LayoutHandle{nullptr, 0};
}
} // namespace dx12
} // namespace SirEngine
