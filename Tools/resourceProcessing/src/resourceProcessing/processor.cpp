#include "processor.h"


#include "animationCompilerPlugin/AnimationCompilerPlugin.h"
#include "SirEngine/log.h"
#include "obj/modelCompilerPlugin.h"
#include "pointTilerCompilerPlugin/pointTilerCompilerPlugin.h"
#include "psoCompilerPlugin/PSOCompilerPlugin.h"
#include "shaderCompilerPlugin/shaderCompilerPlugin.h"
#include "textureCompilerPlugin/textureCompilerPlugin.h"
#include "vkShaderCompilerPlugin/vkShaderCompilerPlugin.h"

namespace SirEngine::ResourceProcessing {
void Processor::initialize() {
  registerFunction("modelCompilerPlugin", processModel);
  registerFunction("PointTilerCompilerPlugin", processPoints);
  registerFunction("shaderCompilerPlugin", processShader);
  registerFunction("vkShaderCompilerPlugin", processVkShader);
  registerFunction("animationCompilerPlugin", processAnim);
  registerFunction("textureCompilerPlugin", processTexture);
  registerFunction("PSOCompilerPlugin", processPSO);

}

void Processor::registerFunction(const std::string& name,
                                 ResourceProcessFunction func) {
  m_registry[name] = func;
}

void Processor::process(const std::string& name, const std::string& fpath,
                        const std::string& opath, const std::string& args) {
  auto found = m_registry.find(name);
  if (found == m_registry.end()) {
    SE_CORE_ERROR("Could not find processor with name {}", name);
    return;
  }

  found->second(fpath, opath, args);
}
}  // namespace SirEngine::ResourceProcessing
