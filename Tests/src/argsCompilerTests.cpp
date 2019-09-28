#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do
                           // this in one cpp file
#include "catch/catch.hpp"
#include "SirEngine/argsUtils.h"

unsigned int Factorial(unsigned int number) {
  return number <= 1 ? number : Factorial(number - 1) * number;
}

TEST_CASE("Split model compile full args", "[argsSplit]") {
  const std::string inargs =
      "-p data/meshes/armorChest.obj -o data/processed/armorChest.model "
      "--pluginName modelCompilerPlugin  --pluginArgs \"--tangents  "
      "data/meshes/armorChest.tangents\"";
  SplitArgs args = splitArgs(inargs);

  REQUIRE(args.argc == 9);
  REQUIRE((*args.storage)[0] == "-p");
  REQUIRE((*args.storage)[1] == "data/meshes/armorChest.obj");
  REQUIRE((*args.storage)[2] == "-o");
  REQUIRE((*args.storage)[3] == "data/processed/armorChest.model");
  REQUIRE((*args.storage)[4] == "--pluginName");
  REQUIRE((*args.storage)[5] == "modelCompilerPlugin");
  REQUIRE((*args.storage)[6] == "--pluginArgs");
  REQUIRE((*args.storage)[7] ==
          "\"--tangents  data/meshes/armorChest.tangents\"");

  // checking the values have the right pointer in argv
  for (int i = 1; i < args.argc; ++i) {
    REQUIRE((*args.storage)[i - 1].c_str() == args.argv[i]);
  }
}

TEST_CASE("Split model compile plugins args", "[argsSplit]") {
  // pluging args are passed removed of the extra quotes
  const std::string inargs = "--tangents data/meshes/armorChest.tangents";
  SplitArgs args = splitArgs(inargs);

  REQUIRE(args.argc == 3);
  REQUIRE((*args.storage)[0] == "--tangents");
  REQUIRE((*args.storage)[1] == "data/meshes/armorChest.tangents");

  // checking the values have the right pointer in argv
  for (int i = 1; i < args.argc; ++i) {
    REQUIRE((*args.storage)[i - 1].c_str() == args.argv[i]);
  }
}

TEST_CASE("Split shader compile full args", "[argsSplit]") {
  const std::string inargs =
      "-p data/shaders/rasterization/basicMeshVS.hlsl -o "
      "data/processed/shaders/rasterization/basicMesh.cso --pluginName "
      "shaderCompilerPlugin --pluginArgs \"-t vs_6_3  -e VS -d \"";
  SplitArgs args = splitArgs(inargs);

  REQUIRE(args.argc == 9);
  REQUIRE((*args.storage)[0] == "-p");
  REQUIRE((*args.storage)[1] == "data/shaders/rasterization/basicMeshVS.hlsl");
  REQUIRE((*args.storage)[2] == "-o");
  REQUIRE((*args.storage)[3] ==
          "data/processed/shaders/rasterization/basicMesh.cso");
  REQUIRE((*args.storage)[4] == "--pluginName");
  REQUIRE((*args.storage)[5] == "shaderCompilerPlugin");
  REQUIRE((*args.storage)[6] == "--pluginArgs");
  REQUIRE((*args.storage)[7] == "\"-t vs_6_3  -e VS -d \"");

  // checking the values have the right pointer in argv
  for (int i = 1; i < args.argc; ++i) {
    REQUIRE((*args.storage)[i - 1].c_str() == args.argv[i]);
  }
}

TEST_CASE("Split shader compile plugins args", "[argsSplit]") {
  // pluging args are passed removed of the extra quotes
  const std::string inargs = "-t vs_6_3  -e VS -d ";
  SplitArgs args = splitArgs(inargs);

  REQUIRE(args.argc == 6);
  REQUIRE((*args.storage)[0] == "-t");
  REQUIRE((*args.storage)[1] == "vs_6_3");
  REQUIRE((*args.storage)[2] == "-e");
  REQUIRE((*args.storage)[3] == "VS");
  REQUIRE((*args.storage)[4] == "-d");

  // checking the values have the right pointer in argv
  for (int i = 1; i < args.argc; ++i) {
    REQUIRE((*args.storage)[i - 1].c_str() == args.argv[i]);
  }
}

TEST_CASE("Split shader compile plugins args with dedicated compiler args",
          "[argsSplit]") {
  // pluging args are passed removed of the extra quotes
  // here is to test if the cargs are parsed properly
  const std::string inargs = "-t cs_6_2  -e CS  -c \"/D AMD\"";
  SplitArgs args = splitArgs(inargs);

  REQUIRE(args.argc == 7);
  REQUIRE((*args.storage)[0] == "-t");
  REQUIRE((*args.storage)[1] == "cs_6_2");
  REQUIRE((*args.storage)[2] == "-e");
  REQUIRE((*args.storage)[3] == "CS");
  REQUIRE((*args.storage)[4] == "-c");
  REQUIRE((*args.storage)[5] == "\"/D AMD\"");

  // checking the values have the right pointer in argv
  for (int i = 1; i < args.argc; ++i) {
    REQUIRE((*args.storage)[i - 1].c_str() == args.argv[i]);
  }
}

TEST_CASE("Split compiler args", "[argsSplit]") {
  // pluging args are passed removed of the extra quotes
  // here is to test if the cargs are parsed properly
  const std::string inargs = "/D AMD";
  SplitArgs args = splitArgs(inargs);

  REQUIRE(args.argc == 3);
  REQUIRE((*args.storage)[0] == "/D");
  REQUIRE((*args.storage)[1] == "AMD");

  // checking the values have the right pointer in argv
  for (int i = 1; i < args.argc; ++i) {
    REQUIRE((*args.storage)[i - 1].c_str() == args.argv[i]);
  }
}
TEST_CASE("get ready compiler args", "[argsSplit]") {
  // pluging args are passed removed of the extra quotes
  // here is to test if the cargs are parsed properly

  SirEngine::globals::STRING_POOL =
      new SirEngine::StringPool(2 << 22); // 4 megabyte allocation
  SirEngine::globals::FRAME_ALLOCATOR = new SirEngine::StackAllocator();
  SirEngine::globals::FRAME_ALLOCATOR->initialize(2 << 22);
  const std::string inargs = "/D AMD";
  std::vector<std::wstring> splitCompilerArgsList;
  SirEngine::ResizableVector<wchar_t *> splitCompilerArgsListPointers;
  splitCompilerArgs(inargs,
                    splitCompilerArgsListPointers);

  // look silly, after all this work we get out to the same value BUT
  // this will work for multiple arguments/defines, which should give
  // us room in the future for shader variants if we ever want them
  REQUIRE(splitCompilerArgsList[0] == std::wstring(L"/D AMD"));
  // release memory
  delete SirEngine::globals::STRING_POOL;
  delete SirEngine::globals::FRAME_ALLOCATOR;
}
