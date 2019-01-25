#include "platform/windows/graphics/dx12/PSOManager.h"

#include "SirEngine/fileUtils.h"
#include "nlohmann/json.hpp"
#include <d3d12.h>

#include "platform/windows/graphics/dx12/dxgiFormatsDefine.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/shaderLayout.h"
#include "platform/windows/graphics/dx12/shaderManager.h"

#include <iostream>

namespace temp{
namespace rendering {

static const std::string PSO_KEY_DXILLIB = "DXILLibrary";
static const std::string PSO_KEY_HIT_GROUPS = "hitGroups";
static const std::string PSO_KEY_HIT_GROUP_NAME = "hitGroupName";
static const std::string PSO_KEY_HIT_CLOSEST_SHADER_NAME = "closestShaderName";
static const std::string PSO_KEY_HIT_ANY_HIT_SHADER_NAME = "anyHitShaderName";
static const std::string PSO_KEY_PAYLOAD_SIZE = "payloadSizeInByte";
static const std::string PSO_KEY_ATTRIBUTE_SIZE = "attributeSizeInByte";
static const std::string PSO_KEY_LOCAL_ROOTS = "localRootSignatures";
static const std::string PSO_KEY_GLOBAL_ROOT = "globalRootSignature";
static const std::string PSO_KEY_ROOT_SIGNATURE_NAME = "rootSignatureName";
static const std::string PSO_KEY_EXPORT_NAME = "exportName";
static const std::string PSO_KEY_MAX_RECURSION = "maxRecursionDepth";
static const std::string PSO_KEY_TYPE = "type";
static const std::string PSO_KEY_TYPE_DXR = "DXR";
static const std::string PSO_KEY_TYPE_RASTER = "RASTER";
static const std::string PSO_KEY_TYPE_COMPUTE = "COMPUTE";
static const std::string PSO_KEY_SHADER_NAME = "shaderName";
static const std::string PSO_KEY_INPUT_LAYOUT = "inputLayout";
static const std::string PSO_KEY_VS_SHADER = "VS";
static const std::string PSO_KEY_PS_SHADER = "PS";
static const std::string PSO_KEY_RASTER_STATE = "rasterState";
static const std::string PSO_KEY_BLEND_STATE = "blendState";
static const std::string PSO_KEY_DEPTH_STENCIL_STATE = "depthStencilState";
static const std::string PSO_KEY_SAMPLEMASK = "sampleMask";
static const std::string PSO_KEY_TOPOLOGY_TYPE = "topologyType";
static const std::string PSO_KEY_RENDER_TARGETS = "renderTargets";
static const std::string PSO_KEY_RTV_FORMATS = "rtvFormats";
static const std::string PSO_KEY_SAMPLE_DESC_COUNT = "sampleDescCount";
static const std::string PSO_KEY_SAMPLE_DESC_QUALITY = "sampleDescQuality";
static const std::string PSO_KEY_DSV_FORMAT = "dsvFormat";
static const std::string DEFAULT_STRING = "";
static const std::string DEFAULT_STATE = "default";
static const int DEFAULT_INT = -1;
static const std::unordered_map<std::string, PSOType> STRING_TO_PSOTYPE{
    {PSO_KEY_TYPE_DXR, PSOType::DXR},
    {PSO_KEY_TYPE_COMPUTE, PSOType::COMPUTE},
    {PSO_KEY_TYPE_RASTER, PSOType::RASTER},
};

// NOTE: the #name means that the macro argument will need to be used as a
// string
// giving us {"name",name}, where "name" is expanded inside the quotes to the
// content
// of the argument name
#define X(name) {#name, name},
static const std::unordered_map<std::string, DXGI_FORMAT> STRING_TO_DXGI_FORMAT{
    LIST_OF_FORMATS};
#undef X

static const std::unordered_map<std::string, D3D12_PRIMITIVE_TOPOLOGY_TYPE>
    STRING_TO_TOPOLOGY = {{"triangle", D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE}};

inline PSOType convertStringPSOTypeToEnum(const std::string &type) {
  auto found = STRING_TO_PSOTYPE.find(type);
  return (found != STRING_TO_PSOTYPE.end() ? found->second : PSOType::INVALID);
}

inline DXGI_FORMAT convertStringToDXGIFormat(const std::string &format) {
  auto found = STRING_TO_DXGI_FORMAT.find(format);
  if (found != STRING_TO_DXGI_FORMAT.end()) {
    return found->second;
  }
  assert(0 && "provided string format is not a valid DXGI_FORMAT");
  return DXGI_FORMAT_UNKNOWN;
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE
convertStringToTopology(const std::string &topology) {
  auto found = STRING_TO_TOPOLOGY.find(topology);
  if (found != STRING_TO_TOPOLOGY.end()) {
    return found->second;
  }
  assert(
      0 &&
      "provided string format is not a valid d3d12 topology, or unsupported");
  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

inline bool isStateDefault(const std::string &state) {
  return state == DEFAULT_STATE;
}

void assertInJson(const nlohmann::json &jobj, const std::string &key) {
  auto found = jobj.find(key);
  assert(found != jobj.end());
}

void PSOManager::init(ID3D12Device4 *device, ShadersLayoutRegistry *registry,
                      RootSignatureManager *root, ShaderManager *shader)
{
  m_dxrDevice = device;
  rs_manager= root;
  shaderManager= shader;
  layoutManger = registry;
}
void PSOManager::cleanup() {
  for (auto pso : m_psoDXRRegister) {
    pso.second->Release();
  }

  for (auto pso : m_psoRegister) {
    pso.second->Release();
  }
  m_psoDXRRegister.clear();
  m_psoRegister.clear();
}
void PSOManager::loadPSOInFolder(const char *directory) {
  std::vector<std::string> paths;
  listFilesInFolder(directory, paths, "json");

  for (const auto &p : paths) {
    loadPSOFile(p.c_str());
  }
}
void PSOManager::loadPSOFile(const char *path) {
  auto jobj = get_json_obj(path);
  std::cout << "[Engine]: Loading PSO from: " << path << std::endl;

  const std::string psoTypeString =
      getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
  assert(!psoTypeString.empty());
  PSOType psoType = convertStringPSOTypeToEnum(psoTypeString);
  switch (psoType) {
  case (PSOType::COMPUTE): {
    processComputePSO(jobj, path);
    break;
  }
  //case (PSOType::DXR): {
  //  processDXRPSO(jobj, path);
  //  break;
  //}
  case (PSOType::RASTER): {
    processRasterPSO(jobj, path);
    break;
  }
  default: {
    assert(0 && "PSO Type not supported");
  }
  }
}

void PSOManager::processComputePSO(nlohmann::json &jobj,
                                   const std::string &path) {

  // lets process the PSO for a compute shader which is quite simple
  const std::string globalRootSignatureName =
      getValueIfInJson(jobj, PSO_KEY_GLOBAL_ROOT, DEFAULT_STRING);
  assert(!globalRootSignatureName.empty());

  const std::string shaderName =
      getValueIfInJson(jobj, PSO_KEY_SHADER_NAME, DEFAULT_STRING);
  assert(!shaderName.empty());

  auto *rootS =
      rs_manager->getRootSignatureFromName(globalRootSignatureName.c_str());

  // fetching the shader from the shader manager, the shader manager contains
  // both rasterization and compute shaders, the DXIL for raytracer are handled
  // by the DXIL manager
  auto *computeShader = shaderManager->getShaderFromName(shaderName);

  D3D12_SHADER_BYTECODE computeShaderByteCode{computeShader->GetBufferPointer(),
                                              computeShader->GetBufferSize()};
  ID3D12PipelineState *pipeStateObject;
  // configure compute shader
  D3D12_COMPUTE_PIPELINE_STATE_DESC cdesc{};
  cdesc.pRootSignature = rootS;
  cdesc.CS = computeShaderByteCode;
  cdesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
  m_dxrDevice->CreateComputePipelineState(&cdesc,
                                          IID_PPV_ARGS(&pipeStateObject));

  std::string name = getFileName(path);
  m_psoRegister[name] = pipeStateObject;
}

inline D3D12_RASTERIZER_DESC getRasterState(const std::string &state) {
  bool rasterDefault = isStateDefault(state);
  if (rasterDefault) {
    return CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  }
  assert(0 && "unsupported raster state");
  return CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
}
inline D3D12_BLEND_DESC getBlendState(const std::string &state) {
  bool isDefault = isStateDefault(state);
  if (isDefault) {
    return CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  }
  assert(0 && "unsupported blend state");
  return CD3DX12_BLEND_DESC(D3D12_DEFAULT);
}
inline D3D12_DEPTH_STENCIL_DESC getDSState(const std::string &state) {
  bool isDefault = isStateDefault(state);
  if (isDefault) {
    return CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  }
  assert(0 && "unsupported depth stencil state");
  return CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
}

void PSOManager::processRasterPSO(nlohmann::json &jobj,
                                  const std::string &path) {

  // find the input layout
  const std::string layoutString =
      getValueIfInJson(jobj, PSO_KEY_INPUT_LAYOUT, DEFAULT_STRING);
  rendering::LayoutHandle layout =
      layoutManger->getShaderLayoutFromName(layoutString);

  const std::string rootSignatureString =
      getValueIfInJson(jobj, PSO_KEY_GLOBAL_ROOT, DEFAULT_STRING);
  auto rootSignature =
      rs_manager->getRootSignatureFromName(rootSignatureString.c_str());

  const std::string VSname =
      getValueIfInJson(jobj, PSO_KEY_VS_SHADER, DEFAULT_STRING);
  const std::string PSname =
      getValueIfInJson(jobj, PSO_KEY_PS_SHADER, DEFAULT_STRING);

  auto *vs = shaderManager->getShaderFromName(VSname);
  auto *ps = shaderManager->getShaderFromName(PSname);

  const std::string rasterStateString =
      getValueIfInJson(jobj, PSO_KEY_RASTER_STATE, DEFAULT_STRING);

  const std::string blendStateString =
      getValueIfInJson(jobj, PSO_KEY_BLEND_STATE, DEFAULT_STRING);

  const std::string depthStencilStateString =
      getValueIfInJson(jobj, PSO_KEY_DEPTH_STENCIL_STATE, DEFAULT_STRING);

  D3D12_RASTERIZER_DESC rasterState = getRasterState(rasterStateString);
  D3D12_BLEND_DESC blendState = getBlendState(blendStateString);
  D3D12_DEPTH_STENCIL_DESC dsState = getDSState(depthStencilStateString);

  const std::string sampleMaskString =
      getValueIfInJson(jobj, PSO_KEY_SAMPLEMASK, DEFAULT_STRING);
  UINT sampleMask = UINT_MAX;
  if (sampleMaskString != "max") {
    assert(0 && "unsupported sample mask");
  }

  const std::string topologyString =
      getValueIfInJson(jobj, PSO_KEY_TOPOLOGY_TYPE, DEFAULT_STRING);
  D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType =
      convertStringToTopology(topologyString);

  int renderTargets =
      getValueIfInJson(jobj, PSO_KEY_RENDER_TARGETS, DEFAULT_INT);

  assertInJson(jobj, PSO_KEY_RTV_FORMATS);
  std::vector<DXGI_FORMAT> formats;
  for (auto &format : jobj[PSO_KEY_RTV_FORMATS]) {
    formats.push_back(convertStringToDXGIFormat(format.get<std::string>()));
  }
  assert(renderTargets == formats.size() &&
         "number of render targets and provided formats don't match");

  int sampleDescCount =
      getValueIfInJson(jobj, PSO_KEY_SAMPLE_DESC_COUNT, DEFAULT_INT);
  int sampleDescQuality =
      getValueIfInJson(jobj, PSO_KEY_SAMPLE_DESC_QUALITY, DEFAULT_INT);

  const std::string dsvFormatString =
      getValueIfInJson(jobj, PSO_KEY_DSV_FORMAT, DEFAULT_STRING);
  DXGI_FORMAT dsvFormat = convertStringToDXGIFormat(dsvFormatString);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
  ID3D12PipelineState *pso = nullptr;
  ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  psoDesc.InputLayout = {layout.layout, (UINT)layout.size};
  psoDesc.pRootSignature = rootSignature;
  psoDesc.VS = {reinterpret_cast<BYTE *>(vs->GetBufferPointer()),
                vs->GetBufferSize()};
  psoDesc.PS = {reinterpret_cast<BYTE *>(ps->GetBufferPointer()),
                ps->GetBufferSize()};
  psoDesc.RasterizerState = rasterState;
  psoDesc.BlendState = blendState;
  psoDesc.DepthStencilState = dsState;
  psoDesc.SampleMask = sampleMask;
  psoDesc.PrimitiveTopologyType = topologyType;
  psoDesc.NumRenderTargets = renderTargets;
  for (int i = 0; i < formats.size(); ++i) {
    psoDesc.RTVFormats[i] = formats[i];
  }
  psoDesc.SampleDesc.Count = sampleDescCount;
  psoDesc.SampleDesc.Quality = sampleDescQuality;
  psoDesc.DSVFormat = dsvFormat;
  HRESULT result =
      m_dxrDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

  assert(SUCCEEDED(result));
  const std::string name = getFileName(path);
  // assert(m_psoRegister.find(name) == m_psoRegister.end());
  m_psoRegister[name] = pso;
}
/*
void PSOManager::processDXRPSO(nlohmann::json &jobj, const std::string &path) {
  CD3DX12_STATE_OBJECT_DESC raytracingPipeline{
      D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};

  const std::string dxilName =
      getValueIfInJson(jobj, PSO_KEY_DXILLIB, DEFAULT_STRING);
  assert(!dxilName.empty());

  assert(jobj.find(PSO_KEY_HIT_GROUPS) != jobj.end());
  auto &hitgroups = jobj[PSO_KEY_HIT_GROUPS];

  processHitGrops(hitgroups, raytracingPipeline);

  // payload
  processPayload(jobj, raytracingPipeline);

  // local root signatures
  processLocalRootSignatures(jobj, raytracingPipeline);

  // global root signature
  processGlobalRootSignature(jobj, raytracingPipeline);

  // pipeline config
  processPipelineConfig(jobj, raytracingPipeline);

  ID3D12StateObject *dxrStateObject;
  printStateObjectDesc(raytracingPipeline);
  HRESULT res = m_dxrDevice->CreateStateObject(raytracingPipeline,
                                               IID_PPV_ARGS(&dxrStateObject));
  assert(res == S_OK);
  std::string name = getFileName(path);
  m_psoDXRRegister[name] = dxrStateObject;
}

void PSOManager::processHitGrops(nlohmann::json &jobj,
                                 CD3DX12_STATE_OBJECT_DESC &pipe) {
  // process all the hitgroups
  for (auto &subj : jobj) {
    std::string hitGroupName =
        getValueIfInJson(subj, PSO_KEY_HIT_GROUP_NAME, DEFAULT_STRING);
    assert(!hitGroupName.empty());

    std::string closestShaderName =
        getValueIfInJson(subj, PSO_KEY_HIT_CLOSEST_SHADER_NAME, DEFAULT_STRING);

    std::string anyHitShaderName =
        getValueIfInJson(subj, PSO_KEY_HIT_ANY_HIT_SHADER_NAME, DEFAULT_STRING);

    auto hitGroup = pipe.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    if (!closestShaderName.empty()) {
      std::wstring closestShaderNameW(closestShaderName.begin(),
                                      closestShaderName.end());
      hitGroup->SetClosestHitShaderImport(closestShaderNameW.c_str());
    }
    if (!anyHitShaderName.empty()) {
      std::wstring anyHitShaderNameW(anyHitShaderName.begin(),
                                     anyHitShaderName.end());
      hitGroup->SetAnyHitShaderImport(anyHitShaderNameW.c_str());
    }

    std::wstring hitGroupNameW(hitGroupName.begin(), hitGroupName.end());
    hitGroup->SetHitGroupExport(hitGroupNameW.c_str());
  }
}
void PSOManager::processPayload(nlohmann::json &jobj,
                                CD3DX12_STATE_OBJECT_DESC &pipe) {
  int defaultInt = -1;
  UINT payloadSize = getValueIfInJson(jobj, PSO_KEY_PAYLOAD_SIZE, defaultInt);
  assert(payloadSize != -1);
  UINT attributeSize =
      getValueIfInJson(jobj, PSO_KEY_ATTRIBUTE_SIZE, defaultInt);
  assert(attributeSize != -1);

  auto shaderConfig =
      pipe.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
  shaderConfig->Config(payloadSize, attributeSize);
}
void PSOManager::processLocalRootSignatures(nlohmann::json &jobj,
                                            CD3DX12_STATE_OBJECT_DESC &pipe) {

  assert(jobj.find(PSO_KEY_LOCAL_ROOTS) != jobj.end());
  auto rootj = jobj[PSO_KEY_LOCAL_ROOTS];

  for (auto &subj : rootj) {
    const std::string rootSignatureName =
        getValueIfInJson(subj, PSO_KEY_ROOT_SIGNATURE_NAME, DEFAULT_STRING);
    assert(!rootSignatureName.empty());

    const std::string exportName =
        getValueIfInJson(subj, PSO_KEY_EXPORT_NAME, DEFAULT_STRING);
    assert(!exportName.empty());

    if (rootSignatureName == "empty") {
      continue;
    }
    auto *raygenS =
        rs_manager->getRootSignatureFromName(rootSignatureName.c_str());
    auto localRootSignature =
        pipe.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    localRootSignature->SetRootSignature(raygenS);
    // m_raytracingLocalRootSignatureRaygen.Get());
    // Shader association
    auto rootSignatureAssociation = pipe.CreateSubobject<
        CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);

    std::wstring exportNameW(exportName.begin(), exportName.end());
    rootSignatureAssociation->AddExport(exportNameW.c_str());
  }
}
*/
void PSOManager::processGlobalRootSignature(nlohmann::json &jobj,
                                            CD3DX12_STATE_OBJECT_DESC &pipe) {
  const std::string globalRootSignatureName =
      getValueIfInJson(jobj, PSO_KEY_GLOBAL_ROOT, DEFAULT_STRING);
  assert(!globalRootSignatureName.empty());

  auto globalRootSignature =
      pipe.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
  auto *globalS =
      rs_manager->getRootSignatureFromName(globalRootSignatureName.c_str());
  globalRootSignature->SetRootSignature(globalS);
}
void PSOManager::processPipelineConfig(nlohmann::json &jobj,
                                       CD3DX12_STATE_OBJECT_DESC &pipe) {

  int defaultInt = -1;
  UINT maxRecursionDepth =
      getValueIfInJson(jobj, PSO_KEY_MAX_RECURSION, defaultInt);
  assert(maxRecursionDepth != -1);
  // Pipeline config
  // Defines the maximum TraceRay() recursion depth.
  auto pipelineConfig =
      pipe.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
  // PERFOMANCE TIP: Set max recursion depth as low as needed
  // as drivers may apply optimization strategies for low recursion depths.
  pipelineConfig->Config(maxRecursionDepth);
}

void PSOManager::printStateObjectDesc(const D3D12_STATE_OBJECT_DESC *desc) {
  std::wstringstream wstr;
  wstr << L"\n";
  wstr << L"-------------------------------------------------------------------"
          L"-\n";
  wstr << L"| D3D12 State Object 0x" << static_cast<const void *>(desc)
       << L": ";
  if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION)
    wstr << L"Collection\n";
  if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE)
    wstr << L"Raytracing Pipeline\n";

  auto ExportTree = [](UINT depth, UINT numExports,
                       const D3D12_EXPORT_DESC *exports) {
    std::wostringstream woss;
    for (UINT i = 0; i < numExports; i++) {
      woss << L"|";
      if (depth > 0) {
        for (UINT j = 0; j < 2 * depth - 1; j++)
          woss << L" ";
      }
      woss << L" [" << i << L"]: ";
      if (exports[i].ExportToRename)
        woss << exports[i].ExportToRename << L" --> ";
      woss << exports[i].Name << L"\n";
    }
    return woss.str();
  };

  for (UINT i = 0; i < desc->NumSubobjects; i++) {
    wstr << L"| [" << i << L"]: ";
    switch (desc->pSubobjects[i].Type) {
    // case D3D12_STATE_SUBOBJECT_TYPE_FLAGS:
    //  wstr << L"Flags (not yet defined)\n";
    //  break;
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
      wstr << L"Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
      break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
      wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
      break;
    case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
      wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8)
           << *static_cast<const UINT *>(desc->pSubobjects[i].pDesc)
           << std::setw(0) << std::dec << L"\n";
      break;
    // case D3D12_STATE_SUBOBJECT_TYPE_CACHED_STATE_OBJECT:
    //  wstr << L"Cached State Object (not yet defined)\n";
    //  break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      wstr << L"DXIL Library 0x";
      auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC *>(
          desc->pSubobjects[i].pDesc);
      wstr << lib->DXILLibrary.pShaderBytecode << L", "
           << lib->DXILLibrary.BytecodeLength << L" bytes\n";
      wstr << ExportTree(1, lib->NumExports, lib->pExports);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      wstr << L"Existing Library 0x";
      auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC *>(
          desc->pSubobjects[i].pDesc);
      wstr << collection->pExistingCollection << L"\n";
      wstr << ExportTree(1, collection->NumExports, collection->pExports);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      wstr << L"Subobject to Exports Association (Subobject [";
      auto association =
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(
              desc->pSubobjects[i].pDesc);
      UINT index = static_cast<UINT>(association->pSubobjectToAssociate -
                                     desc->pSubobjects);
      wstr << index << L"])\n";
      for (UINT j = 0; j < association->NumExports; j++) {
        wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
      }
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      wstr << L"DXIL Subobjects to Exports Association (";
      auto association =
          static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(
              desc->pSubobjects[i].pDesc);
      wstr << association->SubobjectToAssociate << L")\n";
      for (UINT j = 0; j < association->NumExports; j++) {
        wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
      }
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
      wstr << L"Raytracing Shader Config\n";
      auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG *>(
          desc->pSubobjects[i].pDesc);
      wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes
           << L" bytes\n";
      wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes
           << L" bytes\n";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
      wstr << L"Raytracing Pipeline Config\n";
      auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG *>(
          desc->pSubobjects[i].pDesc);
      wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth
           << L"\n";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      wstr << L"Hit Group (";
      auto hitGroup =
          static_cast<const D3D12_HIT_GROUP_DESC *>(desc->pSubobjects[i].pDesc);
      wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]")
           << L")\n";
      wstr << L"|  [0]: Any Hit Import: "
           << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport
                                            : L"[none]")
           << L"\n";
      wstr << L"|  [1]: Closest Hit Import: "
           << (hitGroup->ClosestHitShaderImport
                   ? hitGroup->ClosestHitShaderImport
                   : L"[none]")
           << L"\n";
      wstr << L"|  [2]: Intersection Import: "
           << (hitGroup->IntersectionShaderImport
                   ? hitGroup->IntersectionShaderImport
                   : L"[none]")
           << L"\n";
      break;
    }
    }
    wstr << L"|----------------------------------------------------------------"
            L"----\n";
  }
  wstr << L"\n";
  // OutputDebugStringW(wstr.str().c_str());
  std::wcout << wstr.str() << std::endl;
}
} // namespace rendering
} // namespace dx12