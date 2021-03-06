#include <chrono>
#include <filesystem>

#include "SirEngine/io/argsUtils.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"
#include "nlohmann/json.hpp"
#include "resourceProcessing/processor.h"

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

inline std::string getPluginsArgs(const cxxopts::ParseResult &result) {
  const auto &args = result["pluginArgs"].as<std::string>();
  // lets make sure it does not start or ends with a quote
  int start = 0;
  int end = static_cast<int>(args.length() - 1);
  if (args[0] == '"') {
    ++start;
  }
  if (args[args.length() - 1] == '"') {
    --end;
  }
  std::string out = args.substr(start, end);
  return out;
}
void executeFromArgs(const cxxopts::ParseResult &result,
                     SirEngine::ResourceProcessing::Processor *processor) {
  size_t countIn = result.count("filePath");
  size_t countOut = result.count("outPath");
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

  size_t plugArgsCount = result.count("pluginArgs");
  std::string args;
  if (plugArgsCount != 0) {
    args = getPluginsArgs(result);
  }

  processor->process(name, fpath, opath, args);
}

void executeFile(const cxxopts::ParseResult &result,
                 SirEngine::ResourceProcessing::Processor *processor) {
  const std::string executeFile = result["execute"].as<std::string>();
  nlohmann::json jobj;
  SirEngine::getJsonObj(executeFile, jobj);

  // lets check for the  commands
  if (jobj.find("commands") != jobj.end()) {
    const nlohmann::json &commandsj = jobj["commands"];

    for (auto &command : commandsj) {
      // we now have the command as a string we need to tokenize it
      const auto cs = command.get<std::string>();
      SplitArgs args = splitArgs(cs);
      auto options = getCxxOptions();
      char **argv = args.argv.get();
      auto pluginResult = options.parse(args.argc, argv);
      executeFromArgs(pluginResult, processor);

      // free memory which is not persistent;
      SirEngine::globals::STRING_POOL->resetFrameMemory();
      SirEngine::globals::FRAME_ALLOCATOR->reset();
    }

  } else {
    SE_CORE_ERROR("Could not find command array in json file");
  }
}

int main(int argc, char *argv[]) {
  SirEngine::StringPool stringPool(1024 * 1024 * 10);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::globals::FRAME_ALLOCATOR = new SirEngine::StackAllocator();
  SirEngine::globals::FRAME_ALLOCATOR->initialize(1024 * 1024 * 10);
  SirEngine::ThreeSizesPool pool(1024 * 1024 * 10);
  SirEngine::globals::PERSISTENT_ALLOCATOR = &pool;
  SirEngine::Log::init();

  auto options = getCxxOptions();

  // assert(0);
  SE_CORE_INFO("starting the resource compiler");

  SirEngine::ResourceProcessing::Processor processor;
  processor.initialize();

  const cxxopts::ParseResult result = options.parse(argc, argv);

  const size_t executeCount = result.count("execute");

  const auto start = std::chrono::high_resolution_clock::now();
  if (executeCount) {
    executeFile(result, &processor);
  } else {
    executeFromArgs(result, &processor);
  }
  const auto end = std::chrono::high_resolution_clock::now();
  const auto seconds =
      std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
  SE_CORE_INFO("------------------------------------------------");
  SE_CORE_INFO("Compilation time taken: {0}", seconds);

  SirEngine::Log::free();

  return 0;
}
