#include "platform/windows/graphics/vk/vkPSOManager.h"

#include "SirEngine/application.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkShaderManager.h"

namespace SirEngine::vk {

static const char *PSO_VS_SHADER_ENTRY_POINT = "VS";
static const char *PSO_PS_SHADER_ENTRY_POINT = "PS";
static const char *PSO_CS_SHADER_ENTRY_POINT = "CS";

static const std::string PSO_KEY_GLOBAL_ROOT = "globalRootSignature";
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
static const std::string PSO_KEY_RTV_CLEAR = "clearRTVs";
static const std::string PSO_KEY_RTV_CLEAR_COLOR = "clearRTVCOlors";
static const std::string PSO_KEY_SAMPLE_DESC_COUNT = "sampleDescCount";
static const std::string PSO_KEY_SAMPLE_DESC_QUALITY = "sampleDescQuality";
static const std::string PSO_KEY_DSV_FORMAT = "dsvFormat";
static const std::string PSO_KEY_DSV_CLEAR = "clearDsv";
static const std::string PSO_KEY_DSV_COLOR = "clearDsvColor";
static const std::string PSO_KEY_DEPTH_STENCIL_CONFIG =
    "depthStencilStateConfig";
static const std::string DEFAULT_STRING = "";
static const int DEFAULT_INT = -1;
static const bool DEFAULT_BOOL = false;
static const std::string DEFAULT_STATE = "default";
static const std::string PSO_KEY_CUSTOM_STATE = "custom";
static const std::string PSO_KEY_DEPTH_ENABLED = "depthEnabled";
static const std::string PSO_KEY_DEPTH_WRITE = "depthWrite";
static const std::string PSO_KEY_STENCIL_ENABLED = "stencilEnabled";
static const std::string PSO_KEY_DEPTH_COMPARISON_FUNCTION = "depthFunc";
static const std::string PSO_KEY_RASTER_CONFIG = "rasterStateConfig";
static const std::string PSO_KEY_RASTER_CULL_MODE = "cullMode";
static const std::string PSO_KEY_STENCIL_FAIL_OP = "stencilFailOp";
static const std::string PSO_KEY_STENCIL_DEPTH_FAIL_OP = "stencilDepthFailOp";
static const std::string PSO_KEY_STENCIL_PASS_OP = "stencilPassOp";
static const std::string PSO_KEY_STENCIL_COMPARISON_FUNCTION = "stencilFunc";
static const std::string SWAP_CHAIN_FORMAT_KEY = "SWAP_CHAIN_FORMAT";

static constexpr int MAX_SHADER_STAGE_COUNT = 5;

static const std::unordered_map<std::string, PSO_TYPE> STRING_TO_PSO_TYPE{
    {PSO_KEY_TYPE_DXR, PSO_TYPE::DXR},
    {PSO_KEY_TYPE_COMPUTE, PSO_TYPE::COMPUTE},
    {PSO_KEY_TYPE_RASTER, PSO_TYPE::RASTER},
};
static const std::unordered_map<std::string, VkPrimitiveTopology>
    STRING_TO_TOPOLOGY = {
        {"triangleStrip", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP},
        {"triangle", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
        {"line", VK_PRIMITIVE_TOPOLOGY_LINE_LIST},
};

static const std::unordered_map<std::string, TOPOLOGY_TYPE>
    STRING_TO_ENGINE_TOPOLOGY = {
        {"triangleStrip", TOPOLOGY_TYPE::TRIANGLE_STRIP},
        {"triangle", TOPOLOGY_TYPE::TRIANGLE},
        {"line", TOPOLOGY_TYPE::LINE},
};

static const std::unordered_map<std::string, VkCullModeFlagBits>
    STRING_TO_CULL_MODE_FUNCTION{{"NONE", VK_CULL_MODE_NONE},
                                 {"BACK", VK_CULL_MODE_BACK_BIT},
                                 {"FRONT", VK_CULL_MODE_FRONT_BIT}};

static const std::unordered_map<std::string, VkCompareOp>
    STRING_TO_COMPARISON_FUNCTION{
        {"LESS", VK_COMPARE_OP_LESS},
        {"GREATER", VK_COMPARE_OP_GREATER},
        {"ALWAYS", VK_COMPARE_OP_ALWAYS},
        {"GREATER_EQUAL", VK_COMPARE_OP_GREATER_OR_EQUAL},
        {"EQUAL", VK_COMPARE_OP_EQUAL}};

static const std::unordered_map<std::string, VkStencilOp> STRING_TO_STENCIL_OP{
    {"KEEP", VK_STENCIL_OP_KEEP},
    {"ZERO", VK_STENCIL_OP_ZERO},
    {"REPLACE", VK_STENCIL_OP_REPLACE},
    {"INCR_SAT", VK_STENCIL_OP_INCREMENT_AND_CLAMP},
    {"DECR_SAT", VK_STENCIL_OP_DECREMENT_AND_CLAMP},
    {"INVERT", VK_STENCIL_OP_INVERT},
    {"INCR", VK_STENCIL_OP_INCREMENT_AND_WRAP},
    {"DECR", VK_STENCIL_OP_DECREMENT_AND_WRAP}};

static const std::unordered_map<std::string, VkFormat> STRING_TO_VK_FORMAT{
    {"DXGI_FORMAT_R16G16B16A16_FLOAT", VK_FORMAT_R16G16B16A16_SFLOAT},
    {"DXGI_FORMAT_D32_FLOAT_S8X24_UINT", VK_FORMAT_D32_SFLOAT_S8_UINT},
    {"DXGI_FORMAT_R8G8B8A8_UNORM", VK_FORMAT_R8G8B8A8_UNORM},
    {"DXGI_FORMAT_B8G8R8A8_UNORM", VK_FORMAT_B8G8R8A8_UNORM}};

PSO_TYPE convertStringPSOTypeToEnum(const char *type) {
  const auto found = STRING_TO_PSO_TYPE.find(type);
  return (found != STRING_TO_PSO_TYPE.end() ? found->second
                                            : PSO_TYPE::INVALID);
}

void getShaderStageCreateInfo(const nlohmann::json &jobj,
                              VkPipelineShaderStageCreateInfo *stages,
                              int &shaderStageCount,
                              VkPSOCompileResult &result) {
  // we have a raster pso lets pull out the shader stages
  const std::string type = getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
  if (type == PSO_KEY_TYPE_COMPUTE) {
    const std::string cmpFile =
        getValueIfInJson(jobj, PSO_KEY_SHADER_NAME, DEFAULT_STRING);
    result.CSName = frameString(cmpFile.c_str());
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stages[0].module = vk::SHADER_MANAGER->getShaderFromName(cmpFile.c_str());
    stages[0].pName = PSO_CS_SHADER_ENTRY_POINT;
    stages[0].flags = 0;

  } else {
    const std::string vsFile =
        getValueIfInJson(jobj, PSO_KEY_VS_SHADER, DEFAULT_STRING);
    assert(!vsFile.empty());
    int id = shaderStageCount++;
    // fill up shader binding
    result.VSName = frameString(vsFile.c_str());

    // this allows us to change constants at pipeline creation time,
    // this can allow for example to change compute shaders group size
    // and possibly allow brute force benchmarks with different group sizes(CS
    // only) vsInfo.pSpecializationInfo;
    stages[id].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[id].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[id].module = vk::SHADER_MANAGER->getShaderFromName(vsFile.c_str());
    stages[id].pName = PSO_VS_SHADER_ENTRY_POINT;

    const std::string psFile =
        getValueIfInJson(jobj, PSO_KEY_PS_SHADER, DEFAULT_STRING);
    if (!psFile.empty()) {
      id = shaderStageCount++;

      stages[id].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      stages[id].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      stages[id].module = vk::SHADER_MANAGER->getShaderFromName(psFile.c_str());
      stages[id].pName = PSO_PS_SHADER_ENTRY_POINT;
      result.PSName = frameString(psFile.c_str());
    }
  }
}

inline VkPrimitiveTopology convertStringToTopology(
    const std::string &topology) {
  const auto found = STRING_TO_TOPOLOGY.find(topology);
  if (found != STRING_TO_TOPOLOGY.end()) {
    return found->second;
  }
  assert(0 &&
         "provided string format is not a valid VK topology, or unsupported");
  return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}
inline TOPOLOGY_TYPE convertStringToEngineTopology(
    const std::string &topology) {
  const auto found = STRING_TO_ENGINE_TOPOLOGY.find(topology);
  if (found != STRING_TO_ENGINE_TOPOLOGY.end()) {
    return found->second;
  }
  assert(
      0 &&
      "provided string format is not a valid engine topology, or unsupported");
  return TOPOLOGY_TYPE::UNDEFINED;
}

std::string getAssemblyCreateInfo(
    const nlohmann::json &jobj,
    VkPipelineInputAssemblyStateCreateInfo &inputAssemblyCreateInfo) {
  const std::string topology =
      getValueIfInJson(jobj, PSO_KEY_TOPOLOGY_TYPE, DEFAULT_STRING);
  assert(!topology.empty());

  inputAssemblyCreateInfo.topology = convertStringToTopology(topology);
  assert(inputAssemblyCreateInfo.topology != VK_PRIMITIVE_TOPOLOGY_MAX_ENUM);
  // always false unless geometry shader? geometry shaders are not supported in
  // the engine anyway
  inputAssemblyCreateInfo.primitiveRestartEnable = false;
  return topology;
}

VkCullModeFlagBits getCullMode(const nlohmann::json &jobj) {
  const std::string funcDefault = "BACK";
  const std::string func =
      getValueIfInJson(jobj, PSO_KEY_RASTER_CULL_MODE, funcDefault);
  const auto found = STRING_TO_CULL_MODE_FUNCTION.find(func);
  assert(found != STRING_TO_CULL_MODE_FUNCTION.end());
  return found->second;
}

void getRasterInfo(const nlohmann::json &jobj,
                   VkPipelineRasterizationStateCreateInfo &rasterInfo) {
  const std::string rasterStateString =
      getValueIfInJson(jobj, PSO_KEY_RASTER_STATE, DEFAULT_STRING);

  const bool rasterDefault = rasterStateString == DEFAULT_STATE;
  if (rasterDefault) {
    rasterInfo.depthClampEnable = false;
    rasterInfo.rasterizerDiscardEnable = false;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    // rasterInfo.cullMode = VK_CULL_MODE_NONE;
    rasterInfo.lineWidth = 1.0f;  // even if we don't use it must be specified
    rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    return;
  }
  // if we are here means we have a custom state
  assert(jobj.find(PSO_KEY_RASTER_CONFIG) != jobj.end());
  const auto config = jobj[PSO_KEY_RASTER_CONFIG];

  rasterInfo.depthClampEnable = false;
  rasterInfo.rasterizerDiscardEnable = false;
  rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterInfo.cullMode = getCullMode(config);
  rasterInfo.lineWidth = 1.0f;  // even if we don't use it must be specified
  rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
}

inline VkCompareOp getComparisonFunction(const nlohmann::json &jobj,
                                         const std::string &key) {
  const std::string funcDefault = "LESS";
  const std::string func = getValueIfInJson(jobj, key, funcDefault);
  const auto found = STRING_TO_COMPARISON_FUNCTION.find(func);
  assert(found != STRING_TO_COMPARISON_FUNCTION.end());
  return found->second;
}

inline VkStencilOp getStencilOperationFunction(const nlohmann::json &jobj,
                                               const std::string &key) {
  const std::string funcDefault = "KEEP";
  const std::string func = getValueIfInJson(jobj, key, funcDefault);
  const auto found = STRING_TO_STENCIL_OP.find(func);
  assert(found != STRING_TO_STENCIL_OP.end());
  return found->second;
}

void getDepthStencilState(
    const nlohmann::json jobj,
    VkPipelineDepthStencilStateCreateInfo &depthStencilState) {
  const std::string depthStencilStateString =
      getValueIfInJson(jobj, PSO_KEY_DEPTH_STENCIL_STATE, DEFAULT_STRING);

  const bool isDefault = depthStencilStateString == DEFAULT_STATE;
  if (isDefault) {
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;
    // depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.stencilTestEnable = false;
    return;
  }
  assert(jobj.find(PSO_KEY_DEPTH_STENCIL_CONFIG) != jobj.end());
  const auto dssObj = jobj[PSO_KEY_DEPTH_STENCIL_CONFIG];

  const bool depthEnabled =
      getValueIfInJson(dssObj, PSO_KEY_DEPTH_ENABLED, DEFAULT_BOOL);
  // if not set we retur ntrue
  const bool depthWrite = getValueIfInJson(dssObj, PSO_KEY_DEPTH_WRITE, true);
  depthStencilState.depthTestEnable = depthEnabled;
  depthStencilState.depthWriteEnable = depthWrite;

  depthStencilState.depthCompareOp =
      getComparisonFunction(dssObj, PSO_KEY_DEPTH_COMPARISON_FUNCTION);
  const bool stencilEnabled =
      getValueIfInJson(dssObj, PSO_KEY_STENCIL_ENABLED, DEFAULT_BOOL);
  depthStencilState.stencilTestEnable = stencilEnabled;
  const VkStencilOp stencilFail =
      getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_FAIL_OP);
  const VkStencilOp depthFail =
      getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_DEPTH_FAIL_OP);
  const VkStencilOp stencilPass =
      getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_PASS_OP);
  const VkCompareOp stencilFunction =
      getComparisonFunction(dssObj, PSO_KEY_STENCIL_COMPARISON_FUNCTION);

  depthStencilState.front.compareOp = stencilFunction;
  depthStencilState.front.failOp = stencilFail;
  depthStencilState.front.passOp = stencilPass;
  depthStencilState.front.depthFailOp = depthFail;
  depthStencilState.front.compareMask = 0xff;
  depthStencilState.front.writeMask = 0xff;
  depthStencilState.back = depthStencilState.front;
}

inline VkFormat convertStringToTextureFormat(const std::string &format) {
  if (format == SWAP_CHAIN_FORMAT_KEY) {
    return vk::IMAGE_FORMAT;
  }
  const auto found = STRING_TO_VK_FORMAT.find(format);
  if (found != STRING_TO_VK_FORMAT.end()) {
    return found->second;
  }
  assert(0 && "provided string format is not a valid VK format");
  return VK_FORMAT_UNDEFINED;
}

VkRenderPass getRenderPass(const nlohmann::json &jobj, const char *name) {
  const uint32_t renderTargets =
      getValueIfInJson(jobj, PSO_KEY_RENDER_TARGETS, DEFAULT_INT);

  assertInJson(jobj, PSO_KEY_RTV_FORMATS);

  // TODO change this to maximum number of attachments
  VkAttachmentDescription attachments[10] = {};
  VkAttachmentReference attachmentsRefs[10] = {};

  bool hasClearColors = inJson(jobj, PSO_KEY_RTV_CLEAR);
  auto formats = jobj[PSO_KEY_RTV_FORMATS];
  nlohmann::json::value_type clearColors;
  bool clearDepth = getValueIfInJson(jobj, PSO_KEY_DSV_CLEAR, false);

  if (hasClearColors) {
    clearColors = jobj[PSO_KEY_RTV_CLEAR];
    assert(clearColors.size() == formats.size());
  }

  uint32_t count = 0;
  for (auto &format : formats) {
    VkFormat currentFormat =
        convertStringToTextureFormat(format.get<std::string>());
    assert(currentFormat != VK_FORMAT_UNDEFINED && "Unsupported render format");

    attachments[count].format = currentFormat;
    // TODO no MSAA yet
    attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
    // attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].loadOp = hasClearColors ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                               : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[count].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[count].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // we have only one subpass and uses all attachments
    attachmentsRefs[count].attachment = count;
    attachmentsRefs[count].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    count++;
  }
  assert(renderTargets == count &&
         "number of render targets and provided formats don't match");

  // need to deal with the depth
  const std::string depthStencilStateString =
      getValueIfInJson(jobj, PSO_KEY_DEPTH_STENCIL_STATE, DEFAULT_STRING);
  const std::string dsvFormatString =
      getValueIfInJson(jobj, PSO_KEY_DSV_FORMAT, DEFAULT_STRING);

  const bool isDefault = depthStencilStateString == DEFAULT_STATE;

  bool hasDepthConfig = inJson(jobj, PSO_KEY_DEPTH_STENCIL_CONFIG);
  bool isDepthEnabled = false;
  if (hasDepthConfig) {
    const auto &depthConfig = jobj[PSO_KEY_DEPTH_STENCIL_CONFIG];
    isDepthEnabled =
        getValueIfInJson(depthConfig, PSO_KEY_DEPTH_ENABLED, false);
  }

  bool hasDepth = !dsvFormatString.empty() && (isDefault | isDepthEnabled);

  if (hasDepth) {
    // default state means there is a dsv format to use
    VkFormat currentFormat = convertStringToTextureFormat(dsvFormatString);
    attachments[count].format = currentFormat;
    // TODO no MSAA yet
    attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[count].loadOp =
        clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[count].stencilLoadOp =
        clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[count].initialLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[count].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // we have only one subpass and uses all attachments
    attachmentsRefs[count].attachment = count;
    attachmentsRefs[count].layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    count++;
  }

  VkRenderPass renderPass{};

  VkSubpassDescription subPass{};
  subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subPass.colorAttachmentCount = hasDepth ? count - 1 : count;
  subPass.pColorAttachments = attachmentsRefs;
  subPass.pDepthStencilAttachment =
      hasDepth ? &attachmentsRefs[count - 1] : nullptr;

  VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  createInfo.attachmentCount = count;
  createInfo.pAttachments = attachments;
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subPass;

  vkCreateRenderPass(vk::LOGICAL_DEVICE, &createInfo, nullptr, &renderPass);
  SET_DEBUG_NAME(renderPass, VK_OBJECT_TYPE_RENDER_PASS,
                 frameConcatenation(name, "RenderPass"));
  return renderPass;
}  // namespace SirEngine::vk

PSOHandle VkPSOManager::getHandleFromName(const char *name) const {
  bool found = m_psoRegisterHandle.containsKey(name);
  // assert(found);
  if (!found) {
    SE_CORE_ERROR("could not find PSO handle for name {0}", name);
    return {};
  }
  PSOHandle value{};
  m_psoRegisterHandle.get(name, value);
  return value;
}

PSOHandle VkPSOManager::insertInPSOCache(const VkPSOCompileResult &result) {
  switch (result.psoType) {
    case PSO_TYPE::DXR:
      assert(0);
      break;
    case PSO_TYPE::RASTER: {
      bool hasShader = m_shaderToPSOFile.containsKey(result.VSName);
      if (!hasShader) {
        m_shaderToPSOFile.insert(result.VSName,
                                 new ResizableVector<const char *>(20));
      }
      ResizableVector<const char *> *list;
      m_shaderToPSOFile.get(result.VSName, list);
      // make sure to internalize the string
      list->pushBack(persistentString(result.PSOFullPathFile));

      hasShader = m_shaderToPSOFile.containsKey(result.PSName);
      if (!hasShader) {
        m_shaderToPSOFile.insert(result.PSName,
                                 new ResizableVector<const char *>(20));
      }
      m_shaderToPSOFile.get(result.PSName, list);
      // make sure to internalize the string
      list->pushBack(persistentString(result.PSOFullPathFile));

      // let us find the RS
      const std::string rsName = getFileName(result.rootSignature);
      RSHandle rsHandle =
          vk::PIPELINE_LAYOUT_MANAGER->getHandleFromName(rsName.c_str());

      // generating and storing the handle
      uint32_t index;
      PSOData &data = m_psoPool.getFreeMemoryData(index);
      data.metadata = result.metadata;
      data.pso = result.pso;
      data.topology = result.topologyType;
      data.renderPass = result.renderPass;
      data.layout = result.pipelineLayout;
      data.rootSignature = rsHandle;
      const PSOHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
      data.magicNumber = MAGIC_NUMBER_COUNTER;
      m_psoRegisterHandle.insert(
          persistentString(getFileName(result.PSOFullPathFile).c_str()),
          handle);
      ++MAGIC_NUMBER_COUNTER;
      return handle;
    }
    case PSO_TYPE::COMPUTE: {
      // can probably wrap this into a function to make it less verbose
      // generating and storing the handle
      uint32_t index;
      PSOData &data = m_psoPool.getFreeMemoryData(index);
      data.pso = result.pso;
      data.metadata = result.metadata;
      data.topology = TOPOLOGY_TYPE::UNDEFINED;
      const PSOHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
      data.magicNumber = MAGIC_NUMBER_COUNTER;
      m_psoRegisterHandle.insert(
          persistentString(getFileName(result.PSOFullPathFile).c_str()),
          handle);

      ++MAGIC_NUMBER_COUNTER;

      m_psoRegister.insert(result.CSName, result.pso);

      // need to push the pso to the map
      // first we make sure there is a vector to push to
      const bool hasShader = m_shaderToPSOFile.containsKey(result.CSName);
      if (!hasShader) {
        m_shaderToPSOFile.insert(result.CSName,
                                 new ResizableVector<const char *>(20));
      }
      ResizableVector<const char *> *list;
      m_shaderToPSOFile.get(result.CSName, list);
      // make sure to internalize the string
      list->pushBack(persistentString(result.PSOFullPathFile));

      return handle;
    }
    case PSO_TYPE::INVALID:
      assert(0);
      break;
    default:;
  }
  return {};
}

VkPSOCompileResult VkPSOManager::processRasterPSO(
    const char *filePath, const nlohmann::json &jobj) const {
  // creating a compile result that will be later used for caching
  VkPSOCompileResult compileResult{};
  compileResult.psoType = PSO_TYPE::RASTER;
  compileResult.PSOFullPathFile = frameString(filePath);

  // load root signature
  const std::string fileName = getFileName(filePath);

  graphics::MaterialMetadata metadata =
      graphics::loadMetadata(filePath, GRAPHIC_API::VULKAN);
  // if (fileName == "forwardPhongPSO") {
  //  int x = 0;
  //} else {
  //  metadata = extractMetadata(filePath);
  //}
  RSHandle layoutHandle2 = vk::PIPELINE_LAYOUT_MANAGER->loadSignatureFile(
      fileName.c_str(), &metadata);
  compileResult.rootSignature = frameString(fileName.c_str());
  compileResult.metadata = metadata;

  // RSHandle layoutHandle =
  //    vk::PIPELINE_LAYOUT_MANAGER->loadSignatureFile(rootFile.c_str());
  auto *layout =
      vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(layoutHandle2);

  // load shader stage
  // here we define all the stages of the pipeline
  VkPipelineShaderStageCreateInfo stages[MAX_SHADER_STAGE_COUNT] = {};
  int shaderStageCount = 0;
  getShaderStageCreateInfo(jobj, stages, shaderStageCount, compileResult);

  // no vertex info used, we ditched the input assembler completely
  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  // input assembler info
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  const std::string topology =
      getAssemblyCreateInfo(jobj, inputAssemblyCreateInfo);

  VkPipelineViewportStateCreateInfo viewportCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportCreateInfo.viewportCount = 1;
  viewportCreateInfo.scissorCount = 1;

  // process raster info
  VkPipelineRasterizationStateCreateInfo rasterInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  getRasterInfo(jobj, rasterInfo);

  // TODO support MSAA
  VkPipelineMultisampleStateCreateInfo multisampleState{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencilState = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  getDepthStencilState(jobj, depthStencilState);

  const std::string blendStateString =
      getValueIfInJson(jobj, PSO_KEY_BLEND_STATE, DEFAULT_STRING);
  assert(blendStateString == "default" &&
         "no supported blend state other than default");

  VkRenderPass renderPass = getRenderPass(jobj, fileName.c_str());

  VkPipelineColorBlendAttachmentState attachState{};
  attachState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo blendState{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  blendState.attachmentCount = 1;
  blendState.pAttachments = &attachState;

  // always support dynamic viewport and scissor such that we can resize without
  // recompile PSOs
  VkDynamicState dynStateFlags[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicState.dynamicStateCount =
      sizeof(dynStateFlags) / sizeof(dynStateFlags[0]);
  dynamicState.pDynamicStates = dynStateFlags;

  // create the actual pipline
  VkGraphicsPipelineCreateInfo createInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  createInfo.stageCount = shaderStageCount;
  createInfo.pStages = stages;
  // we are not using vertex state, but vertex pulling
  createInfo.pVertexInputState = &vertexInputCreateInfo;
  createInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
  createInfo.pViewportState = &viewportCreateInfo;
  createInfo.pRasterizationState = &rasterInfo;
  createInfo.pMultisampleState = &multisampleState;
  createInfo.pDepthStencilState = &depthStencilState;
  createInfo.pColorBlendState = &blendState;
  createInfo.pDynamicState = &dynamicState;
  createInfo.renderPass = renderPass;
  createInfo.layout = layout;

  VkPipeline pipeline = nullptr;
  VkResult status = vkCreateGraphicsPipelines(vk::LOGICAL_DEVICE, nullptr, 1,
                                              &createInfo, nullptr, &pipeline);
  assert(status == VK_SUCCESS);
  assert(pipeline);
  SET_DEBUG_NAME(pipeline, VK_OBJECT_TYPE_PIPELINE,
                 frameConcatenation(fileName.c_str(), "Pipeline"));

  //// all good we need to store the data
  //// generating and storing the handle
  // uint32_t index;
  // PSOData &data = m_psoPool.getFreeMemoryData(index);
  // data.pso = pipeline;
  // data.renderPass = renderPass;
  // data.rootSignature = layoutHandle;
  // const PSOHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  // data.magicNumber = MAGIC_NUMBER_COUNTER;
  // data.topology = convertStringToEngineTopology(topology);
  // m_psoRegisterHandle.insert(fileName.c_str(), handle);
  //++MAGIC_NUMBER_COUNTER;

  compileResult.topologyType = convertStringToEngineTopology(topology);
  compileResult.pso = pipeline;
  compileResult.renderPass = renderPass;
  compileResult.pipelineLayout = layout;
  return compileResult;
};

void VkPSOManager::initialize() {
  ENGINE_PIPELINE_LAYOUT = vk::PIPELINE_LAYOUT_MANAGER->createEngineLayout(
      PER_FRAME_LAYOUT, STATIC_SAMPLERS_LAYOUT);
}

void VkPSOManager::cleanup() {
  int count = m_psoRegister.binCount();
  for (int i = 0; i < count; ++i) {
    if (m_psoRegisterHandle.isBinUsed(i)) {
      const char *key = m_psoRegisterHandle.getKeyAtBin(i);
      PSOHandle handle{};
      m_psoRegisterHandle.get(key, handle);
      // now that we have the handle we can get the data
      assertMagicNumber(handle);
      const uint32_t index = getIndexFromHandle(handle);
      const PSOData &data = m_psoPool.getConstRef(index);
      vkDestroyPipeline(vk::LOGICAL_DEVICE, data.pso, nullptr);
      vkDestroyRenderPass(vk::LOGICAL_DEVICE, data.renderPass, nullptr);
    }
  }

  vkDestroyPipelineLayout(vk::LOGICAL_DEVICE, ENGINE_PIPELINE_LAYOUT, nullptr);
  vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, PER_FRAME_LAYOUT, nullptr);
}

void VkPSOManager::loadRawPSOInFolder(const char *directory) {
  std::vector<std::string> paths;
  listFilesInFolder(directory, paths, "json");
  for (const auto &p : paths) {
    auto compileResult = compileRawPSO(p.c_str());
    insertInPSOCache(compileResult);
  }
}

void VkPSOManager::loadCachedPSOInFolder(const char *) { assert(0); }

VkPSOCompileResult VkPSOManager::processComputePSO(
    const char *filePath, const nlohmann::json &jobj) const {
  // creating a compile result that will be later used for caching
  VkPSOCompileResult compileResult{};
  compileResult.psoType = PSO_TYPE::COMPUTE;
  compileResult.PSOFullPathFile = frameString(filePath);

  // load root signature
  const std::string rootFile =
      getValueIfInJson(jobj, PSO_KEY_GLOBAL_ROOT, DEFAULT_STRING);
  assert(!rootFile.empty());

  const std::string fileName = getFileName(filePath);

  graphics::MaterialMetadata metadata =
      graphics::loadMetadata(filePath, GRAPHIC_API::VULKAN);
  RSHandle layoutHandle2 = vk::PIPELINE_LAYOUT_MANAGER->loadSignatureFile(
      fileName.c_str(), &metadata);
  compileResult.rootSignature = frameString(fileName.c_str());
  compileResult.metadata = metadata;

  // RSHandle layoutHandle =
  //    vk::PIPELINE_LAYOUT_MANAGER->loadSignatureFile(rootFile.c_str());
  auto *layout =
      vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(layoutHandle2);

  VkPipelineShaderStageCreateInfo stage{};
  int shaderStageCount = 0;
  getShaderStageCreateInfo(jobj, &stage, shaderStageCount, compileResult);

  VkPipelineCreateFlags pipelineFlags = 0;

  VkComputePipelineCreateInfo pipelineInfo = {
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      nullptr,
      pipelineFlags,
      stage,
      layout,
      nullptr,
      0};

  VkPipeline pipeline = nullptr;
  VkResult result = vkCreateComputePipelines(
      vk::LOGICAL_DEVICE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);

  assert(result == VK_SUCCESS);
  assert(pipeline);
  SET_DEBUG_NAME(pipeline, VK_OBJECT_TYPE_PIPELINE,
                 frameConcatenation(fileName.c_str(), "Pipeline"));

  compileResult.pso = pipeline;
  compileResult.renderPass = nullptr;
  compileResult.pipelineLayout = layout;
  return compileResult;
}

VkPSOCompileResult VkPSOManager::compileRawPSO(const char *file) {
  const std::string fileName = getFileName(file);
  auto jobj = getJsonObj(file);

  const std::string typeString =
      getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
  assert(!typeString.empty());
  PSO_TYPE type = convertStringPSOTypeToEnum(typeString.c_str());

  switch (type) {
    case PSO_TYPE::DXR: {
      assert(0 && "Unsupported PSO type");
      break;
    }
    case PSO_TYPE::RASTER: {
      return processRasterPSO(file, jobj);
    }
    case PSO_TYPE::COMPUTE: {
      return processComputePSO(file, jobj);
    }
    case PSO_TYPE::INVALID: {
      assert(0 && "Unsupported PSO type");
      break;
    }
    default: {
      assert(0);
    }
  }
  return {};
}

void VkPSOManager::recompilePSOFromShader(const char *shaderName,
                                          const char *offsetPath) {
  std::cout << "requested shader reload " << shaderName << std::endl;

  // clearing the log
  compileLog = "";
  ResizableVector<const char *> *psos;
  bool found = m_shaderToPSOFile.get(shaderName, psos);
  if (!found) {
    assert(0);
    return;
  }
  // now we need to extract the data out of the pso to figure out which
  // shaders to recompile
  std::vector<std::string> shadersToRecompile;
  shadersToRecompile.reserve(10);
  int psoCount = psos->size();
  for (int i = 0; i < psoCount; ++i) {
    const char *pso = (*psos)[i];
    const auto jobj = getJsonObj(pso);
    std::cout << "[Engine]: Loading PSO from: " << pso << std::endl;

    const std::string psoTypeString =
        getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
    const PSO_TYPE psoType = convertStringPSOTypeToEnum(psoTypeString.c_str());
    switch (psoType) {
      case (PSO_TYPE::COMPUTE): {
        const std::string computeName =
            getValueIfInJson(jobj, PSO_KEY_SHADER_NAME, DEFAULT_STRING);
        assert(!computeName.empty());
        shadersToRecompile.push_back(computeName);
        break;
      }
      // case (PSOType::DXR): {
      //  processDXRPSO(jobj, path);
      //  break;
      //}
      case (PSO_TYPE::RASTER): {
        std::string vs =
            getValueIfInJson(jobj, PSO_KEY_VS_SHADER, DEFAULT_STRING);
        assert(!vs.empty());
        std::string ps =
            getValueIfInJson(jobj, PSO_KEY_PS_SHADER, DEFAULT_STRING);
        assert(!ps.empty());
        shadersToRecompile.push_back(vs);
        shadersToRecompile.push_back(ps);
        break;
      }
      default: {
        assert(0 && "PSO type not supported");
      }
    }
  }

  // recompile all the shaders involved
  for (auto &shader : shadersToRecompile) {
    bool result = false;
    const char *log =
        vk::SHADER_MANAGER->recompileShader(shader.c_str(), offsetPath, result);
    if (log != nullptr) {
      compileLog += log;
    }
    if (!result) {
      SE_CORE_ERROR("Error in compiling shader {0}", shader);
      // we need to update the log with the error and return
      auto *e = new ShaderCompileResultEvent(compileLog.c_str());
      globals::APPLICATION->queueEventForEndOfFrame(e);
      return;
    }
  }
  // now that all shaders are recompiled we can recompile the pso
  // before doing that we do need to flush to make sure none of the PSO are
  // used
  globals::RENDERING_CONTEXT->flush();

  for (int i = 0; i < psoCount; ++i) {
    const char *pso = (*psos)[i];
    const auto result = compileRawPSO(pso);

    // need to update the cache
    updatePSOCache(getFileName(result.PSOFullPathFile).c_str(), result);

    // log
    compileLog += "Compiled PSO: ";
    compileLog += pso;
    compileLog += "\n";
  }

  // all the shader have been recompiled, we should be able to
  // recompile the PSO now
  auto *e = new ShaderCompileResultEvent(compileLog.c_str());
  globals::APPLICATION->queueEventForEndOfFrame(e);
}

void VkPSOManager::updatePSOCache(const char *name,
                                  const VkPSOCompileResult &result) {
  assert(m_psoRegisterHandle.containsKey(name));
  PSOHandle handle;
  m_psoRegisterHandle.get(name, handle);
  uint32_t index = getIndexFromHandle(handle);
  PSOData &data = m_psoPool[index];
  // release old one
  vkDestroyPipeline(vk::LOGICAL_DEVICE, data.pso, nullptr);
  vkDestroyRenderPass(vk::LOGICAL_DEVICE, data.renderPass, nullptr);
  data.pso = result.pso;
  data.renderPass = result.renderPass;
}

}  // namespace SirEngine::vk
