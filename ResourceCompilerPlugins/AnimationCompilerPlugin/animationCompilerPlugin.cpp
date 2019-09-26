#include "AnimationCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "SirEngine/animation/skeleton.h"
#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include <filesystem>
const std::string PLUGIN_NAME = "animationCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

struct AnimData
{
	std::string name;
	std::vector<SirEngine::JointPose>poses;
	float frameRate;
	int bonesPerFrame;
	int frameCount;
	bool isLoopable;
};

void processArgs(const std::string &args) {
  // nothing do to here for now
  return;
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("Animation compiler",
                           "Converts animations to game ready binary blob");
}

void convertAnim(const std::string &path, AnimData& data) {

  // TODO compile this with resource compiler
  auto jObj = getJsonObj(path);
  // we first check the name because if the name is in the cache
  // we just get out
  data.name = jObj["name"].get<std::string>();

  // querying the basic data
  data.isLoopable = jObj["looping"].get<bool>();

  // this are the maya start and end frame, not used in the engine, leaving
  // them here as reference
  // int start = j_obj["start"].get<int>();
  // int end = j_obj["end"].get<int>();
  data.frameRate = jObj["frame_rate"].get<float>();

  const int posesSize = int(jObj["poses"].size());
  data.bonesPerFrame = int(jObj["bonesPerPose"]);
  data.poses.resize(posesSize * data.bonesPerFrame);

  // reinterpret_cast<SirEngine::JointPose
  // *>(globals::PERSISTENT_ALLOCATOR->allocate(
  //     posesSize * bonesPerFrame * sizeof(SirEngine::JointPose)));
  data.frameCount = posesSize;

  const DirectX::XMFLOAT3 zeroV(0, 0, 0);
  const DirectX::XMVECTOR zeroQ = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

  int poseCounter = 0;
  for (auto &pose : jObj["poses"]) {
    const int jointSize = int(pose.size());
    assert(jointSize == data.bonesPerFrame);

    auto *joints = data.poses.data() + (poseCounter * jointSize);

    int jointCounter = 0;
    for (auto &joint : pose) {

      const DirectX::XMFLOAT3 position =
          getValueIfInJson<DirectX::XMFLOAT3>(joint, "pos", zeroV);

      // extracting joint quaternion
      // to note I export quaternion as x,y,z,w,  is initialized as
      // w,x,y,z
      const DirectX::XMVECTOR rotation =
          getValueIfInJson<DirectX::XMVECTOR>(joint, "quat", zeroQ);

      joints[jointCounter].m_rot = rotation;
      joints[jointCounter].m_trans = position;

      // scale hardcoded, not used for the time being
      joints[jointCounter].m_scale = 1.0f;

      ++jointCounter;
    }

    ++poseCounter;
  }
}

bool processAnim(const std::string &assetPath, const std::string &outputPath,
                 const std::string &args) {

  // processing plugins args
  processArgs(args);

  // checking IO files exits
  bool exits = fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[Animation Compiler] : could not find path/file {0}",
                  assetPath);
  }

  exits = filePathExists(outputPath);
  if (!exits) {
    SE_CORE_ERROR("[Animation Compiler] : could not find path/file {0}",
                  outputPath);
  }

  // loading the obj
  AnimData data{};
  convertAnim(assetPath,data);

  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::ANIM;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  std::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string();
  request.outPath = outputPath.c_str();

  //need to merge the data, name and poses
  std::vector<char> outputData;
  const int nameSize =  static_cast<int>(data.name.size() +1);
  const int posesSize = static_cast<int>(data.poses.size()* sizeof(SirEngine::JointPose));
  const int totalSize = nameSize + posesSize;
  outputData.resize(totalSize);

  //copying the data over
  memcpy(outputData.data(), data.name.data(),nameSize);
  memcpy(outputData.data()+ nameSize, data.poses.data(),posesSize);

  request.bulkData = outputData.data();
  request.bulkDataSizeInByte = totalSize;

  ClipMapperData mapperData;
  mapperData.nameSizeInByte= static_cast<int>(data.name.size()+1);
  mapperData.posesSizeInByte= static_cast<int>(data.poses.size())*sizeof(SirEngine::JointPose);
  mapperData.frameRate = data.frameRate;
  mapperData.bonesPerFrame= data.bonesPerFrame;
  mapperData.frameCount= data.frameCount;
  mapperData.isLoopable= data.isLoopable;
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(ClipMapperData);

  writeBinaryFile(request);

  SE_CORE_INFO("animation successfully compiled ---> {0}", outputPath);

  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processAnim);
  return true;
}
