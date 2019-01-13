#include "cxxopts/cxxopts.hpp"

#include "SirEngine/log.h"
#include "resourceCompilerLib/resourcePlugin.h"

#include <filesystem>
#include <iostream>

const std::string getExecutablePath() {
  HMODULE hModule = GetModuleHandleW(NULL);
  WCHAR path[MAX_PATH];
  GetModuleFileName(hModule, path, MAX_PATH);
  auto exp_path = std::experimental::filesystem::path(path);
  return exp_path.parent_path().string();
}

int main(int argc, char *argv[]) {

  cxxopts::Options options("ResourceCompiler 1.0",
                           "Converts assets in game ready binary blob");
  options.add_options()("p,filePath", "Path of the asset to be processed",
                        cxxopts::value<std::string>())
					   ("o,outPath", "output path for the generate file(s)",
						cxxopts::value<std::string>())
					   ( "pluginName", "Name of the plug-in dealing with the asset",
						cxxopts::value<std::string>())
					   ("pluginArgs", "Arguments to be forwarded to the plug-in",
          cxxopts::value<std::string>());

  SirEngine::Log::init();

  SE_CORE_INFO("starting the resource compiler");
  PluginRegistry::init();

  PluginRegistry *registry = PluginRegistry::getInstance();

  const std::string basePath = getExecutablePath();
  registry->loadPluginsInFolder(basePath + "/plugins");

  auto result = options.parse(argc, argv);
  int countIn = result.count("filePath");
  int countOut = result.count("outPath");
  if (countIn == 0) {
    SE_CORE_ERROR("No filePath provided");
    return 0;
  }
  if (countOut == 0) {
    SE_CORE_ERROR("No output path provided");
    return 0;
  }

  // we now know that we have the proper in/out paths
  const std::string fpath = result["filePath"].as<std::string>();
  const std::string opath = result["outPath"].as<std::string>();

  const std::string name = result["pluginName"].as<std::string>();

  int plugArgsCount = result.count("pluginArgs");
  std::string args = "";
  if (plugArgsCount != 0) {
    args = result["pluginArgs"].as<std::string>();
  }

  ResourceProcessFunction func = registry->getFunction(name);
  assert(func != nullptr);

  func(fpath, opath, args);
  return 0;
}
