#include "modelCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include "processObj.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include <filesystem>
const std::string PLUGIN_NAME = "modelCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

void processArgs(const std::string &args, std::string &tangentPath,
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
  const auto result = options.parse(v.argc, argv);

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
  std::string tangentsPath;
  std::string skinPath;
  processArgs(args, tangentsPath, skinPath);

  // checking IO files exits
  bool exits = SirEngine::fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[Model Compiler] : could not find path/file {0}", assetPath);
  }

  exits = SirEngine::filePathExists(outputPath);
  if (!exits) {
    SE_CORE_ERROR("[Model Compiler] : could not find path/file {0}",
                  outputPath);
  }

  /*
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
    return false;
  }
  */

  // processing the model so that is ready for the GPU
  Model model;
  SkinData finalSkinData;
  // convertObj(attr, shapes[0], model, finalSkinData, tangentsPath, skinPath);
  bool objLoadResult = convertObj(assetPath.c_str(), tangentsPath.c_str(),
                                  skinPath.c_str(), finalSkinData, model);
  assert(objLoadResult);

  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::MODEL;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  std::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string();
  request.outPath = outputPath.c_str();

  /*
  //OLD WAY
  // need to merge indices and vertices
  std::vector<float> data;
  uint32_t stride = 12;
  uint32_t floatVertexCount = static_cast<uint32_t>(model.vertexCount) * stride;
  size_t indicesCount = model.indices.size();
  size_t totalSizeFloat = floatVertexCount + indicesCount;
  size_t totalSizeByte = totalSizeFloat * sizeof(float);

  data.resize(totalSizeFloat);
  memcpy(data.data(), model.vertices.data(), floatVertexCount * sizeof(float));
  // stride is in float being data a float ptr
  memcpy(data.data() + floatVertexCount, model.indices.data(),
         indicesCount * sizeof(float));
  request.bulkData = data.data();
  request.bulkDataSizeInByte = totalSizeByte;

  ModelMapperData mapperData;
  mapperData.indexDataSizeInByte =
      static_cast<unsigned int>(indicesCount * sizeof(float));
  mapperData.vertexDataSizeInByte = floatVertexCount * sizeof(float);
  mapperData.strideInByte = stride * sizeof(float);
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(ModelMapperData);
  for (int i = 0; i < 6; ++i) {
    mapperData.boundingBox[i] = model.boundingBox[i];
  }

  writeBinaryFile(request);

  SE_CORE_INFO("Model successfully compiled ---> {0}", outputPath);

  */

  std::vector<float> data;
  auto floatVertexCount = static_cast<int>(model.vertices.size());
  size_t indicesCount = model.indices.size();
  size_t totalSizeFloat = floatVertexCount + indicesCount;
  size_t totalSizeByte = totalSizeFloat * sizeof(float);

  data.resize(totalSizeFloat);
  memcpy(data.data(), model.vertices.data(), floatVertexCount * sizeof(float));
  // stride is in float being data a float ptr
  memcpy(data.data() + floatVertexCount, model.indices.data(),
         indicesCount * sizeof(float));
  request.bulkData = data.data();
  request.bulkDataSizeInByte = totalSizeByte;

  ModelMapperData mapperData;
  mapperData.indexDataSizeInByte =
      static_cast<unsigned int>(indicesCount * sizeof(float));
  mapperData.vertexDataSizeInByte = floatVertexCount * sizeof(float);
  mapperData.vertexCount = model.vertexCount;
  mapperData.indexCount= static_cast<uint32_t>(indicesCount);
  mapperData.positionRange = model.positionRange;
  mapperData.normalsRange= model.normalsRange;
  mapperData.uvRange= model.uvRange;
  mapperData.tangentsRange=model.tangentsRange;
  mapperData.flags = model.flags;

  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(ModelMapperData);
  for (int i = 0; i < 6; ++i) {
    mapperData.boundingBox[i] = model.boundingBox[i];
  }

  writeBinaryFile(request);

  SE_CORE_INFO("Model successfully compiled ---> {0}", outputPath);

  // if there is a skin data we need to save it aswell
  if (skinPath.empty()) {
    return true;
  }

  // writing binary file
  BinaryFileWriteRequest skinRequest;
  skinRequest.fileType = BinaryFileType::SKIN;
  skinRequest.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  std::string skinOut = outputPath;
  std::string::size_type pos = 0u;
  std::string toReplace = ".model";
  std::string newReplace = ".skin";
  while ((pos = skinOut.find(toReplace, pos)) != std::string::npos) {
    skinOut.replace(pos, toReplace.length(), newReplace);
    pos += newReplace.length();
  }
  skinRequest.outPath = skinOut.c_str();

  // need to merge indices and vertices
  std::vector<float> skinData;
  uint32_t jointCount = static_cast<uint32_t>(finalSkinData.jnts.size());
  uint32_t weightsCount = static_cast<uint32_t>(finalSkinData.weights.size());
  uint32_t totalSkinSizeByte = (jointCount + weightsCount) * sizeof(float);

  skinData.resize(jointCount + weightsCount);
  memcpy(skinData.data(), finalSkinData.jnts.data(),
         jointCount * sizeof(float));
  // stride is in float being data a float ptr
  memcpy(skinData.data() + jointCount, finalSkinData.weights.data(),
         weightsCount * sizeof(float));
  skinRequest.bulkData = skinData.data();
  skinRequest.bulkDataSizeInByte = totalSkinSizeByte;

  SkinMapperData skinMapperData{};
  // TODO HARDCODED, should move this to globals?
  skinMapperData.influenceCountPerVertex = 6;
  skinMapperData.jointsSizeInByte = jointCount * sizeof(float);
  skinMapperData.weightsSizeInByte = weightsCount * sizeof(float);
  skinRequest.mapperData = &skinMapperData;
  skinRequest.mapperDataSizeInByte = sizeof(SkinMapperData);

  writeBinaryFile(skinRequest);

  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processModel);
  return true;
}
