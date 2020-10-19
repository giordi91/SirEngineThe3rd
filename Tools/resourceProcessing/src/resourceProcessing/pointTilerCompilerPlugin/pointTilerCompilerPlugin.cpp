#include "pointTilerCompilerPlugin.h"

#include <filesystem>

#include "SirEngine/io/argsUtils.h"
#include "SirEngine/io/binaryFile.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"
#include "nlohmann/json.hpp"

using namespace std::string_literals;

const std::string PLUGIN_NAME = "PointTilerCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

struct PointsData {
  std::string name;
  uint32_t tileCount;
  uint32_t pointsPerTile;
  std::vector<float> points;
};

bool convertPoints(const std::string &path, PointsData &data) {
  // TODO compile this with resource compiler
  data.name = SirEngine::getFileName(path);

  nlohmann::json jObj;
  SirEngine::getJsonObj(path, jObj);
  const std::string tileKey = "tiles";
  SirEngine::assertInJson(jObj, tileKey);
  const auto &tilesJ = jObj[tileKey];
  data.tileCount = static_cast<uint32_t>(tilesJ.size());
  std::cout << "tile count" << data.tileCount << std::endl;
  if (data.tileCount <= 0) {
    SE_CORE_ERROR("Not point tiles in the file");
    return false;
  }
  size_t pointInTile = tilesJ[0].size();
  data.points.reserve(pointInTile * data.tileCount * 2);
  data.pointsPerTile = static_cast<uint32_t>(pointInTile);

  for (uint32_t i = 0; i < data.tileCount; ++i) {
    const auto &tileData = tilesJ[i];
    assert(tileData.size() == pointInTile);

    for (uint32_t p = 0; p < pointInTile; ++p) {
      const auto &point = tileData[p];
      data.points.push_back(point[0].get<float>());
      data.points.push_back(point[1].get<float>());
    }
  }
  return true;
}

bool processPoints(const std::string &assetPath, const std::string &outputPath,
                   const std::string &) {
  // checking IO files exits
  bool exits = SirEngine::fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[Point Tiler Compiler] : could not find path/file {0}",
                  assetPath);
  }

  exits = SirEngine::filePathExists(outputPath);
  if (!exits) {
    SE_CORE_ERROR("[Point Tiler Compiler] : could not find path/file {0}",
                  outputPath);
  }
  // initialize memory pools and loggers

  // loading the obj
  PointsData data{};
  bool result = convertPoints(assetPath, data);
  if (!result) {
    return false;
  }

  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::POINT_TILER;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  std::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string();
  request.outPath = outputPath.c_str();

  // need to merge the data, name and poses
  std::vector<char> outputData;
  const int nameSize = static_cast<int>(data.name.size() + 1);
  const int pointsSize = static_cast<int>(data.points.size() * sizeof(float));
  const int totalSize = nameSize + pointsSize;
  outputData.resize(totalSize);

  // copying the data over
  memcpy(outputData.data(), data.name.data(), nameSize);
  memcpy(outputData.data() + nameSize, data.points.data(), pointsSize);

  request.bulkData = outputData.data();
  request.bulkDataSizeInByte = totalSize;

  PointTilerMapperData mapperData;
  mapperData.nameSizeInByte = static_cast<int>(data.name.size() + 1);
  mapperData.pointsSizeInByte = pointsSize;
  mapperData.tileCount = data.tileCount;
  mapperData.pointsPerTile = data.pointsPerTile;
  request.mapperData = &mapperData;

  request.mapperDataSizeInByte = sizeof(PointTilerMapperData);

  writeBinaryFile(request);

  SE_CORE_INFO("points tiler successfully compiled ---> {0}", outputPath);

  return true;
}

