#include "modelCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"
#include <iostream>

#include "SirEngine/binary/binaryFile.h"
#include "processObj.h"
#include "resourceCompilerLib/argsUtils.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include <filesystem>
const std::string PLUGIN_NAME = "modelCompilerPlugin";
const unsigned int versionMajor = 0;
const unsigned int versionMinor = 1;
const unsigned int versionPatch = 0;

void processArgs(const std::string args, std::string &tangentPath,
                 std::string &skinPath) {
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("Model compiler",
                           "Converts a model in game ready binary blob");
  options.add_options()("tangents", "Path to the tangent file",
                        cxxopts::value<std::string>())(
      "skin", "Path to the skin cluster", cxxopts::value<std::string>());
  char **argv = v.argv.get();
  auto result = options.parse(v.argc, argv);

  if (result.count("tangents")) {
    tangentPath = result["tangents"].as<std::string>();
  }
  if (result.count("skin") != 0) {
    skinPath = result["skin"].as<std::string>();
  }
}

bool processModel(const std::string &assetPath, const std::string &outputPath,
                  const std::string &args) {

  // processing plugins args
  std::string tangentsPath = "";
  std::string skinPath = "";
  processArgs(args, tangentsPath, skinPath);

  // checking IO files exits
  bool exits = fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[Model Compiler] : could not find path/file {0}", assetPath);
  }

  exits = filePathExists(outputPath);
  std::cout << "outputPath " << outputPath << std::endl;
  if (!exits) {
    SE_CORE_ERROR("[Model Compiler] : could not find path/file {0}",
                  outputPath);
  }

  // loading the obj
  tinyobj::attrib_t attr;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string warn;
  std::string err;
  bool ret = tinyobj::LoadObj(&attr, &shapes, &materials, &warn, &err,
                              assetPath.c_str());
  if (!ret) {
    SE_CORE_ERROR("Error in parsing obj file {0}", assetPath);
    return false ;
  }

  // processing the model so that is ready for the GPU
  Model model;
  convertObj(attr, shapes[0], model, tangentsPath, skinPath);

  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = 1;
  request.version = ((versionMajor << 16) | (versionMinor << 8) | versionPatch);

  std::experimental::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string().c_str();
  const std::string outFilePath = outputPath;
  request.outPath = outFilePath.c_str();

  // need to merge indices and vertices
  std::vector<float> data;
  int floatVertexCount = model.vertexCount * 16;
  size_t indicesCount = model.indices.size();
  size_t totalSizeFloat = floatVertexCount + indicesCount;
  size_t totalSizeByte = totalSizeFloat * sizeof(float);

  data.resize(totalSizeFloat);
  memcpy(data.data(), model.vertices.data(), floatVertexCount * sizeof(float));
  // stride is in float being data a float ptr
  memcpy(data.data() + floatVertexCount, model.indices.data(),
         indicesCount * sizeof(float));
  request.bulkData = data.data();
  request.bulkDataSizeInBtye = totalSizeByte;

  ModelMapperData mapperData;
  mapperData.indexDataSizeInByte =
      static_cast<unsigned int>(indicesCount * sizeof(float));
  mapperData.vertexDataSizeInByte = floatVertexCount * sizeof(float);
  mapperData.strideInByte = 16 * sizeof(float);
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(ModelMapperData);

  writeBinaryFile(request);
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processModel);
  return true;
}
