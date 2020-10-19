#pragma once
#include <string>
#include <unordered_map>

namespace SirEngine::ResourceProcessing {
typedef bool (*ResourceProcessFunction)(const std::string &assetPath,
                                        const std::string &outputPath,
                                        const std::string &pluginArgs);

class Processor
{
public:
  void initialize();
  void registerFunction(const std::string &name,
                               ResourceProcessFunction func);
  void process(const std::string& name, const std::string& fpath, const std::string& opath, const std::string& args);

  std::unordered_map<std::string, ResourceProcessFunction> m_registry;
};

}
