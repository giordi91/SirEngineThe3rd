#include "textureCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "processTexture.h"
#include "resourceCompilerLib/argsUtils.h"

const std::string PLUGIN_NAME = "textureCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

void processArgs(const std::string args, std::string &format) {
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("Texture compiler",
                           "Converts a texture in DDS format");
  options.add_options()("f,format", "output format for the texture",
                        cxxopts::value<std::string>());
  char **argv = v.argv.get();
  auto result = options.parse(v.argc, argv);

  if (result.count("format")) {
    format = result["format"].as<std::string>();
  }
}

bool processTexture(const std::string &assetPath, const std::string &outputPath,
                    const std::string &args) {

  // processing plugins args
  std::string format = "";
  processArgs(args, format);

  // checking IO files exits
  bool exits = fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[Texture Compiler] : could not find path/file {0}",
                  assetPath);
  }

  exits = filePathExists(outputPath);
  if (!exits) {
    SE_CORE_ERROR("[Texture Compiler] : could not find path/file {0}",
                  outputPath);
  }
  bool res = loadTextureFromFile(assetPath.c_str(), outputPath.c_str(), format);

  if (res) {
    SE_CORE_INFO("Texture successfully compiled ---> {0}", outputPath);
  }
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processTexture);
  return true;
}
