#include "platform/windows/graphics/dx12/PSOCompile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/nodes/framePassDebugNode.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/dxgiFormatsDefine.h"
#include "rootSignatureCompile.h"
#include "shaderCompiler.h"
#include "shaderLayout.h"

namespace SirEngine::dx12 {
static const std::string PSO_KEY_GLOBAL_ROOT = "globalRootSignature";
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
static const std::string PSO_KEY_SAMPLE_MASK = "sampleMask";
static const std::string PSO_KEY_TOPOLOGY_TYPE = "topologyType";
static const std::string PSO_KEY_RENDER_TARGETS = "renderTargets";
static const std::string PSO_KEY_RTV_FORMATS = "rtvFormats";
static const std::string PSO_KEY_SAMPLE_DESC_COUNT = "sampleDescCount";
static const std::string PSO_KEY_SAMPLE_DESC_QUALITY = "sampleDescQuality";
static const std::string PSO_KEY_DSV_FORMAT = "dsvFormat";
static const std::string PSO_KEY_DEPTH_STENCIL_CONFIG =
    "depthStencilStateConfig";
static const std::string DEFAULT_STRING = "";
static const std::string DEFAULT_STATE = "default";
static const std::string PSO_KEY_CUSTOM_STATE = "custom";
static const std::string PSO_KEY_DEPTH_ENABLED = "depthEnabled";
static const std::string PSO_KEY_STENCIL_ENABLED = "stencilEnabled";
static const std::string PSO_KEY_DEPTH_COMPARISON_FUNCTION = "depthFunc";
static const std::string PSO_KEY_RASTER_CONFIG = "rasterStateConfig";
static const std::string PSO_KEY_RASTER_CULL_MODE = "cullMode";
static const std::string PSO_KEY_STENCIL_FAIL_OP = "stencilFailOp";
static const std::string PSO_KEY_STENCIL_DEPTH_FAIL_OP = "stencilDepthFailOp";
static const std::string PSO_KEY_STENCIL_PASS_OP = "stencilPassOp";
static const std::string PSO_KEY_STENCIL_COMPARISON_FUNCTION = "stencilFunc";

static const int DEFAULT_INT = -1;
static const bool DEFAULT_BOOL = false;
static const std::unordered_map<std::string, PSOType> STRING_TO_PSO_TYPE{
    {PSO_KEY_TYPE_DXR, PSOType::DXR},
    {PSO_KEY_TYPE_COMPUTE, PSOType::COMPUTE},
    {PSO_KEY_TYPE_RASTER, PSOType::RASTER},
};

static const std::unordered_map<std::string, D3D12_COMPARISON_FUNC>
    STRING_TO_COMPARISON_FUNCTION{
        {"LESS", D3D12_COMPARISON_FUNC_LESS},
        {"GREATER", D3D12_COMPARISON_FUNC_GREATER},
        {"ALWAYS", D3D12_COMPARISON_FUNC_ALWAYS},
        {"GREATER_EQUAL", D3D12_COMPARISON_FUNC_GREATER_EQUAL},
        {"EQUAL", D3D12_COMPARISON_FUNC_EQUAL}};

static const std::unordered_map<std::string, D3D12_CULL_MODE>
    STRING_TO_CULL_MODE_FUNCTION{{"NONE", D3D12_CULL_MODE_NONE},
                                 {"BACK", D3D12_CULL_MODE_BACK}};

static const std::unordered_map<std::string, D3D12_STENCIL_OP>
    STRING_TO_STENCIL_OP{{"KEEP", D3D12_STENCIL_OP_KEEP},
                         {"ZERO", D3D12_STENCIL_OP_ZERO},
                         {"REPLACE", D3D12_STENCIL_OP_REPLACE},
                         {"INCR_SAT", D3D12_STENCIL_OP_INCR_SAT},
                         {"DECR_SAT", D3D12_STENCIL_OP_DECR_SAT},
                         {"INVERT", D3D12_STENCIL_OP_INVERT},
                         {"INCR", D3D12_STENCIL_OP_INCR},
                         {"DECR", D3D12_STENCIL_OP_DECR}};

// NOTE: the #name means that the macro argument will need to be used as a
// string  giving us {"name",name}, where "name" is expanded inside the quotes
// to the content of the argument name
#define X(name) {#name, name},
static const std::unordered_map<std::string, DXGI_FORMAT> STRING_TO_DXGI_FORMAT{
    LIST_OF_FORMATS};
#undef X

static const std::unordered_map<std::string, D3D12_PRIMITIVE_TOPOLOGY_TYPE>
    STRING_TO_TOPOLOGY = {
        {"triangle", D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE},
        {"line", D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE},
};

PSOType convertStringPSOTypeToEnum(const char *type) {
  const auto found = STRING_TO_PSO_TYPE.find(type);
  return (found != STRING_TO_PSO_TYPE.end() ? found->second : PSOType::INVALID);
}

inline DXGI_FORMAT convertStringToDXGIFormat(const std::string &format) {
  const auto found = STRING_TO_DXGI_FORMAT.find(format);
  if (found != STRING_TO_DXGI_FORMAT.end()) {
    return found->second;
  }
  assert(0 && "provided string format is not a valid DXGI_FORMAT");
  return DXGI_FORMAT_UNKNOWN;
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE
convertStringToTopology(const std::string &topology) {
  const auto found = STRING_TO_TOPOLOGY.find(topology);
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
  const auto found = jobj.find(key);
  assert(found != jobj.end());
}
D3D12_CULL_MODE getCullMode(const nlohmann::json &jobj) {
  const std::string funcDefault = "BACK";
  const std::string func =
      getValueIfInJson(jobj, PSO_KEY_RASTER_CULL_MODE, funcDefault);
  const auto found = STRING_TO_CULL_MODE_FUNCTION.find(func);
  assert(found != STRING_TO_CULL_MODE_FUNCTION.end());
  return found->second;
}
inline D3D12_RASTERIZER_DESC getRasterState(const std::string &state,
                                            const nlohmann::json &jobj) {
  const bool rasterDefault = isStateDefault(state);
  if (rasterDefault) {
    return CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  }
  assert(jobj.find(PSO_KEY_RASTER_CONFIG) != jobj.end());
  const auto config = jobj[PSO_KEY_RASTER_CONFIG];
  auto desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

  const D3D12_CULL_MODE cullMode = getCullMode(config);
  desc.CullMode = cullMode;
  return desc;
}
inline D3D12_BLEND_DESC getBlendState(const std::string &state) {
  const bool isDefault = isStateDefault(state);
  if (isDefault) {
    return CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  }
  assert(0 && "unsupported blend state");
  return CD3DX12_BLEND_DESC(D3D12_DEFAULT);
}

inline bool isStateCustom(const std::string &state) {
  return state == PSO_KEY_CUSTOM_STATE;
}

inline D3D12_COMPARISON_FUNC getComparisonFunction(const nlohmann::json &jobj,
                                                   const std::string &key) {
  const std::string funcDefault = "LESS";
  const std::string func = getValueIfInJson(jobj, key, funcDefault);
  const auto found = STRING_TO_COMPARISON_FUNCTION.find(func);
  assert(found != STRING_TO_COMPARISON_FUNCTION.end());
  return found->second;
}

inline D3D12_STENCIL_OP getStencilOperationFunction(const nlohmann::json &jobj,
                                                    const std::string &key) {
  const std::string funcDefault = "KEEP";
  const std::string func = getValueIfInJson(jobj, key, funcDefault);
  const auto found = STRING_TO_STENCIL_OP.find(func);
  assert(found != STRING_TO_STENCIL_OP.end());
  return found->second;
}
inline D3D12_DEPTH_STENCIL_DESC getDSState(const std::string &state,
                                           const nlohmann::json &jobj) {
  const bool isDefault = isStateDefault(state);
  if (isDefault) {
    return CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  }
  if (isStateCustom(state)) {
    CD3DX12_DEPTH_STENCIL_DESC desc;
    assert(jobj.find(PSO_KEY_DEPTH_STENCIL_CONFIG) != jobj.end());
    const auto dssObj = jobj[PSO_KEY_DEPTH_STENCIL_CONFIG];

    const bool depthEnabled =
        getValueIfInJson(dssObj, PSO_KEY_DEPTH_ENABLED, DEFAULT_BOOL);
    desc.DepthEnable = depthEnabled;
    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc =
        getComparisonFunction(dssObj, PSO_KEY_DEPTH_COMPARISON_FUNCTION);
    const bool stencilEnabled =
        getValueIfInJson(dssObj, PSO_KEY_STENCIL_ENABLED, DEFAULT_BOOL);
    desc.StencilEnable = stencilEnabled;
    desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_STENCIL_OP stencilFail =
        getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_FAIL_OP);
    const D3D12_STENCIL_OP depthFail =
        getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_DEPTH_FAIL_OP);
    const D3D12_STENCIL_OP stencilPass =
        getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_PASS_OP);
    const D3D12_COMPARISON_FUNC stencilFunction =
        getComparisonFunction(dssObj, PSO_KEY_STENCIL_COMPARISON_FUNCTION);

    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
        stencilFail, depthFail, stencilPass, stencilFunction};
    desc.FrontFace = defaultStencilOp;
    desc.BackFace = defaultStencilOp;
    return desc;
  }
  assert(0 && "unsupported depth stencil state");
  return CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
}
std::string getComputeShaderNameFromPSO(const nlohmann::json &jobj) {
  const std::string shaderName =
      getValueIfInJson(jobj, PSO_KEY_SHADER_NAME, DEFAULT_STRING);
  assert(!shaderName.empty());
  return shaderName;
}

void getRasterShaderNameFromPSO(const nlohmann::json &jobj, std::string &vs,
                                std::string &ps) {
  vs = getValueIfInJson(jobj, PSO_KEY_VS_SHADER, DEFAULT_STRING);
  assert(!vs.empty());
  ps = getValueIfInJson(jobj, PSO_KEY_PS_SHADER, DEFAULT_STRING);
  assert(!ps.empty());
}

// void processGlobalRootSignature(
//    nlohmann::json &jobj, CD3DX12_STATE_OBJECT_DESC &pipe) {
//  const std::string globalRootSignatureName =
//      getValueIfInJson(jobj, PSO_KEY_GLOBAL_ROOT, DEFAULT_STRING);
//  assert(!globalRootSignatureName.empty());
//
//  auto globalRootSignature =
//      pipe.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
//  auto *globalS =
//      rs_manager->getRootSignatureFromName(globalRootSignatureName.c_str());
//  globalRootSignature->SetRootSignature(globalS);
//}
// void PSOManager::processPipelineConfig(nlohmann::json &jobj,
//                                       CD3DX12_STATE_OBJECT_DESC &pipe) const
//                                       {
//  int defaultInt = -1;
//  int maxRecursionDepth =
//      getValueIfInJson(jobj, PSO_KEY_MAX_RECURSION, defaultInt);
//  assert(maxRecursionDepth != -1);
//  // Pipeline config
//  // Defines the maximum TraceRay() recursion depth.
//  auto pipelineConfig =
//      pipe.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
//  // PERFORMANCE TIP: Set max recursion depth as low as needed
//  // as drivers may apply optimization strategies for low recursion depths.
//  pipelineConfig->Config(maxRecursionDepth);
//}

PSOCompileResult processComputePSO(nlohmann::json &jobj,
                                   const std::string &path) {
  // lets process the PSO for a compute shader which is quite simple
  const std::string globalRootSignatureName =
      getValueIfInJson(jobj, PSO_KEY_GLOBAL_ROOT, DEFAULT_STRING);
  assert(!globalRootSignatureName.empty());

  const std::string shaderName =
      getValueIfInJson(jobj, PSO_KEY_SHADER_NAME, DEFAULT_STRING);
  assert(!shaderName.empty());

  // ID3D12RootSignature *rootS =
  //    rs_manager->getRootSignatureFromName(globalRootSignatureName.c_str());

  auto resultCompile = processSignatureFile(globalRootSignatureName.c_str());
  auto rootS = resultCompile.root;

  
  DXCShaderCompiler m_compiler;
  // fetching the shader from the shader manager, the shader manager contains
  // both rasterization and compute shaders, the DXIL for raytracer are
  // handled by the DXIL manager
  ShaderArgs csArgs;
  csArgs.entryPoint = L"CS";
  csArgs.debug = true;
  csArgs.type = L"cs_6_2";


  std::string log;
  auto *computeShader = m_compiler.compileShader(shaderName.c_str(),csArgs,&log);

  D3D12_SHADER_BYTECODE computeShaderByteCode{computeShader->GetBufferPointer(),
                                              computeShader->GetBufferSize()};
  ID3D12PipelineState *pso;
  // configure compute shader
  D3D12_COMPUTE_PIPELINE_STATE_DESC cdesc{};
  cdesc.pRootSignature = rootS;
  cdesc.CS = computeShaderByteCode;
  cdesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
  dx12::DEVICE->CreateComputePipelineState(&cdesc, IID_PPV_ARGS(&pso));

  const std::string name = getFileName(path);
  return PSOCompileResult{pso,
                          PSOType::COMPUTE,
                          nullptr,
                          nullptr,
                          frameString(name.c_str()),
                          frameString(name.c_str()),
                          frameString(path.c_str())};
}

PSOCompileResult processRasterPSO(nlohmann::json &jobj,
                                  const std::string &path) {
  // find the input layout
  const std::string layoutString =
      getValueIfInJson(jobj, PSO_KEY_INPUT_LAYOUT, DEFAULT_STRING);
  SirEngine::dx12::LayoutHandle layout =
      dx12::SHADER_LAYOUT_REGISTRY->getShaderLayoutFromName(layoutString);

  const std::string rootSignatureString =
      getValueIfInJson(jobj, PSO_KEY_GLOBAL_ROOT, DEFAULT_STRING);
  // auto rootSignature =
  //    rs_manager->getRootSignatureFromName(rootSignatureString.c_str());

  auto resultCompile = processSignatureFile(rootSignatureString.c_str());
  auto rootSignature = resultCompile.root;

  const std::string VSname =
      getValueIfInJson(jobj, PSO_KEY_VS_SHADER, DEFAULT_STRING);
  const std::string PSname =
      getValueIfInJson(jobj, PSO_KEY_PS_SHADER, DEFAULT_STRING);


  DXCShaderCompiler m_compiler;
  ShaderArgs vsArgs;
  vsArgs.entryPoint = L"VS";
  vsArgs.debug = true;
  vsArgs.type = L"vs_6_2";

  std::string log;
  auto *vs = m_compiler.compileShader(VSname.c_str(),vsArgs,&log);


  vsArgs.entryPoint = L"PS";
  vsArgs.debug = true;
  vsArgs.type = L"ps_6_2";
  ID3DBlob* ps = nullptr;
  if(PSname != "null") {
	ps = m_compiler.compileShader(PSname.c_str(),vsArgs,&log);
  }

  const std::string rasterStateString =
      getValueIfInJson(jobj, PSO_KEY_RASTER_STATE, DEFAULT_STRING);

  const std::string blendStateString =
      getValueIfInJson(jobj, PSO_KEY_BLEND_STATE, DEFAULT_STRING);

  const std::string depthStencilStateString =
      getValueIfInJson(jobj, PSO_KEY_DEPTH_STENCIL_STATE, DEFAULT_STRING);

  D3D12_RASTERIZER_DESC rasterState = getRasterState(rasterStateString, jobj);
  D3D12_BLEND_DESC blendState = getBlendState(blendStateString);
  D3D12_DEPTH_STENCIL_DESC dsState = getDSState(depthStencilStateString, jobj);

  const std::string sampleMaskString =
      getValueIfInJson(jobj, PSO_KEY_SAMPLE_MASK, DEFAULT_STRING);
  UINT sampleMask = UINT_MAX;
  if (sampleMaskString != "max") {
    assert(0 && "unsupported sample mask");
  }

  const std::string topologyString =
      getValueIfInJson(jobj, PSO_KEY_TOPOLOGY_TYPE, DEFAULT_STRING);
  D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType =
      convertStringToTopology(topologyString);

  size_t renderTargets =
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
  psoDesc.InputLayout = {layout.layout, static_cast<UINT>(layout.size)};
  psoDesc.pRootSignature = rootSignature;
  psoDesc.VS = {reinterpret_cast<BYTE *>(vs->GetBufferPointer()),
                vs->GetBufferSize()};

  psoDesc.PS = {nullptr, 0};
  if (ps != nullptr) {
    psoDesc.PS = {reinterpret_cast<BYTE *>(ps->GetBufferPointer()),
                  ps->GetBufferSize()};
  }

  psoDesc.RasterizerState = rasterState;
  psoDesc.BlendState = blendState;
  psoDesc.DepthStencilState = dsState;
  psoDesc.SampleMask = sampleMask;
  psoDesc.PrimitiveTopologyType = topologyType;
  psoDesc.NumRenderTargets = static_cast<UINT>(renderTargets);
  for (size_t i = 0; i < formats.size(); ++i) {
    psoDesc.RTVFormats[i] = formats[i];
  }
  psoDesc.SampleDesc.Count = sampleDescCount;
  psoDesc.SampleDesc.Quality = sampleDescQuality;
  psoDesc.DSVFormat = dsvFormat;
  HRESULT result =
      dx12::DEVICE->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

  assert(SUCCEEDED(result));
  const std::string name = getFileName(path);
  // assert(m_psoRegister.find(name) == m_psoRegister.end());
  // m_psoRegister.insert(name.c_str(), pso);

  return PSOCompileResult{pso,
                          PSOType::RASTER,
                          frameString(VSname.c_str()),
                          frameString(PSname.c_str()),
                          nullptr,
                          frameString(name.c_str()),
                          frameString(path.c_str())};
} // namespace SirEngine::dx12

PSOCompileResult loadPSOFile(const char *path) {
  auto jobj = getJsonObj(path);
  SE_CORE_INFO("[Engine]: Loading PSO from: {0}", path);

  const std::string psoTypeString =
      getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
  assert(!psoTypeString.empty());
  const PSOType psoType = convertStringPSOTypeToEnum(psoTypeString.c_str());

  switch (psoType) {
  case (PSOType::COMPUTE): {
    return processComputePSO(jobj, path);
    break;
  }
  case (PSOType::DXR): {
    assert(0);
    break;
  }
  case (PSOType::RASTER): {
    return processRasterPSO(jobj, path);
    break;
  }
  default: {
    assert(0 && "PSO Type not supported");
    break;
  }
  }
  return PSOCompileResult{nullptr, PSOType::INVALID};
}

} // namespace SirEngine::dx12
