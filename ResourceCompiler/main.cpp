#include "SirEngine/fileUtils.h"
#include "cxxopts/cxxopts.hpp"
#include "nlohmann/json.hpp"
#include <fstream>

#include "SirEngine/log.h"
#include "resourceCompilerLib/argsUtils.h"
#include "resourceCompilerLib/resourcePlugin.h"

#include <filesystem>
#include <iostream>

inline cxxopts::Options getCxxOptions() {
  cxxopts::Options options("ResourceCompiler 1.0",
                           "Converts assets in game ready binary blob");
  options.add_options()("p,filePath", "Path of the asset to be processed",
                        cxxopts::value<std::string>())(
      "o,outPath", "output path for the generate file(s)",
      cxxopts::value<std::string>())(
      "pluginName", "Name of the plug-in dealing with the asset",
      cxxopts::value<std::string>())("pluginArgs",
                                     "Arguments to be forwarded to the plug-in",
                                     cxxopts::value<std::string>())(
      "e,execute", "pass in a bundle of commands in json format",
      cxxopts::value<std::string>());
  return options;
}

const std::string getExecutablePath() {
  HMODULE hModule = GetModuleHandleW(NULL);
  WCHAR path[MAX_PATH];
  GetModuleFileName(hModule, path, MAX_PATH);
  auto exp_path = std::experimental::filesystem::path(path);
  return exp_path.parent_path().string();
}
inline nlohmann::json getJsonObj(std::string path) {

  bool res = fileExists(path);
  if (res) {
    // let s open the stream
    std::ifstream st(path);
    std::stringstream s_buffer;
    s_buffer << st.rdbuf();
    std::string s_buff = s_buffer.str();

    try {
      // try to parse
      nlohmann::json j_obj = nlohmann::json::parse(s_buff);
      return j_obj;
    } catch (...) {
      // if not lets throw an error
      SE_CORE_ERROR("Error parsing json file from path {0}", path);
      auto ex = std::current_exception();
      ex._RethrowException();
      return nlohmann::json();
    }
  } else {
    assert(0);
    return nlohmann::json();
  }
}

inline std::string getPluginsArgs(const cxxopts::ParseResult &result) {
  const std::string &args = result["pluginArgs"].as<std::string>();
  // lest smake sure it does not start or ends with a quote
  int start = 0;
  int end = args.length()-1;
  if (args[0] == '"') {
    ++start;
  }
  if (args[args.length() - 1] == '"') {
    --end;
  }
  std::string out= args.substr(start, end);
  return out;
}
void executeFromArgs(const cxxopts::ParseResult &result) {
  int countIn = result.count("filePath");
  int countOut = result.count("outPath");
  if (countIn == 0) {
    SE_CORE_ERROR("No filePath provided");
    return;
  }
  if (countOut == 0) {
    SE_CORE_ERROR("No output path provided");
    return;
  }

  // we now know that we have the proper in/out paths
  const std::string fpath = result["filePath"].as<std::string>();
  const std::string opath = result["outPath"].as<std::string>();

  const std::string name = result["pluginName"].as<std::string>();

  int plugArgsCount = result.count("pluginArgs");
  std::string args = "";
  if (plugArgsCount != 0) {
    args = getPluginsArgs(result);
  }

  PluginRegistry *registry = PluginRegistry::getInstance();
  ResourceProcessFunction func = registry->getFunction(name);
  assert(func != nullptr);

  func(fpath, opath, args);
}

void executeFile(const std::string &compilerPath,
                 const cxxopts::ParseResult &result) {
  const std::string executeFile = result["execute"].as<std::string>();
  const nlohmann::json jobj = getJsonObj(executeFile);

  // lets check for the  commands
  if (jobj.find("commands") != jobj.end()) {
    const nlohmann::json &commandsj = jobj["commands"];
    for (auto &command : commandsj) {
      // we now have the command as a string we need to tokenize it
      const std::string cs = command.get<std::string>();
      SplitArgs args = splitArgs(cs);
      auto options = getCxxOptions();
      char **argv = args.argv.get();
      auto result = options.parse(args.argc, argv);
      executeFromArgs(result);
    }

  } else {
    SE_CORE_ERROR("Could not find command array in json file");
  }
}

int main(int argc, char *argv[]) {

  SirEngine::Log::init();
  auto options = getCxxOptions();

  SE_CORE_INFO("starting the resource compiler");
  PluginRegistry::init();

  PluginRegistry *registry = PluginRegistry::getInstance();

  const std::string basePath = getExecutablePath();
  registry->loadPluginsInFolder(basePath + "/plugins");

  const cxxopts::ParseResult result = options.parse(argc, argv);

  int executeCount = result.count("execute");

  if (executeCount) {
    executeFile(basePath, result);
  } else {
    executeFromArgs(result);
  }

  return 0;
}
