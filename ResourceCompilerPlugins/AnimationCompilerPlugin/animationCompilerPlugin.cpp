#include "AnimationCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "SirEngine/animation/animationManager.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include <filesystem>
#include "nlohmann/json.hpp"

using namespace std::string_literals;

const std::string PLUGIN_NAME = "animationCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

struct AnimData {
  std::string name;
  std::vector<SirEngine::JointPose> poses;
  float frameRate;
  int bonesPerFrame;
  int frameCount;
  bool isLoopable;
  // holds the metadata for the animation, where the first int is
  // the keyword and the vector is the frames affected by that keyword
  std::unordered_map<int, std::vector<int>> animationKeywordToFrame;
};

void processArgs(const std::string &) {
  // nothing do to here for now
  return;
}

bool isNumber(const std::string &s) {
  return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) {
                         return !std::isdigit(c);
                       }) == s.end();
}
void convertAnim(const std::string &path, AnimData &data) {

  nlohmann::json jObj;
  SirEngine::getJsonObj(path, jObj);
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
  data.frameCount = posesSize;

  const glm::vec3 zeroV(0, 0, 0);
  const glm::quat zeroQ(0.0f, 0.0f, 0.0f, 0.0f);

  int poseCounter = 0;
  for (auto &pose : jObj["poses"]) {
    const int jointSize = int(pose.size());
    assert(jointSize == data.bonesPerFrame);

    auto *joints = data.poses.data() + (poseCounter * jointSize);

    int jointCounter = 0;
    for (auto &joint : pose) {

      const glm::vec3 position =
          SirEngine::getValueIfInJson(joint, "pos", zeroV);

      // extracting joint quaternion
      // to note I export quaternion as x,y,z,w,  is initialized as
      // w,x,y,z
      auto rotation =
          SirEngine::getValueIfInJson(joint, "quat", zeroQ);

      joints[jointCounter].m_rot = rotation;
      joints[jointCounter].m_trans = position;

      // scale hardcoded, not used for the time being
      joints[jointCounter].m_scale = 1.0f;

      ++jointCounter;
    }

    ++poseCounter;
  }

  // processing metadata
  // auto reString = R"("(\d+)-?(\d+)?")";
  auto reString = R"((\d+)-?(\d+)?)";
  std::regex re(reString);
  if (jObj.find("metadata") == jObj.end()) {
    // no metadata just return
    return;
  }
  auto metadata = jObj["metadata"];
  for (auto keyValue : metadata) {
    const auto key = keyValue[0].get<std::string>();
    const auto value = keyValue[1].get<std::string>();

    // converting key to int
    int keyInt =
        SirEngine::globals::ANIMATION_MANAGER->animationKeywordNameToValue(
            key.c_str());
    if (keyInt == -1) {
      SE_CORE_WARN("Could not map key to value: {0}", key);
    }

    // we need to process the value
    std::smatch matches;
    if (std::regex_search(value, matches, re)) {
      // if matches size 1 is one number only
      // std::cout<<matches.size()<<std::endl;
      int size = static_cast<int>(matches.size());
      assert(size == 3);

      const std::string match0 = matches[0].str();
      const std::string match1 = matches[1].str();
      const std::string match2 = matches[2].str();

      bool match1Empty = match1.empty();
      bool match2Empty = match2.empty();

      // TODO: bit too nested for my likings, might have to revisit in the
      // future
      if (match1Empty || !isNumber(match1)) {
        SE_CORE_WARN("Could not parse key value:  {0}:{1}, match1 failed", key,
                     value);
        continue;
      }
      int match1Int = strtol(match1.c_str(), nullptr, 10);
      if (!match2Empty) {
        if (!isNumber(match2)) {
          SE_CORE_WARN("Could not parse key value:  {0}:{1}, match2 failed",
                       key, value);
          continue;
        }
        // it means we are dealing with a range
        int match2Int = strtol(match2.c_str(), nullptr, 10);

        // now we need to check whether the start frame is less than the end
        // frame
        if (match2Int < match1Int) {
          // now we have a wrap around happening we need to do two loops
          for (int i = match1Int; i < data.frameCount; ++i) {
            data.animationKeywordToFrame[keyInt].push_back(i);
          }
          for (int i = 0; i < match2Int; ++i) {
            data.animationKeywordToFrame[keyInt].push_back(i);
          }

        } else {

          for (int i = match1Int; i < match2Int; ++i) {
            data.animationKeywordToFrame[keyInt].push_back(i);
          }
        }

      } else {
        data.animationKeywordToFrame[keyInt].push_back(match1Int);
      }
    } else {
      SE_CORE_WARN("Could not parse key value:  {0}:{1}", key, value);
    }
  }
  // finally lets sort all the keys
  for (auto &keyValue : data.animationKeywordToFrame) {
    std::sort(keyValue.second.begin(), keyValue.second.end());
  }
}

bool processAnim(const std::string &assetPath, const std::string &outputPath,
                 const std::string &args) {

  // processing plugins args
  processArgs(args);

  // checking IO files exits
  bool exits = SirEngine::fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[Animation Compiler] : could not find path/file {0}",
                  assetPath);
  }

  exits = SirEngine::filePathExists(outputPath);
  if (!exits) {
    SE_CORE_ERROR("[Animation Compiler] : could not find path/file {0}",
                  outputPath);
  }
  // initialize memory pools and loggers

  SirEngine::AnimationManager animManager;
  animManager.init();
  SirEngine::globals::ANIMATION_MANAGER = &animManager;

  // loading the obj
  AnimData data{};
  convertAnim(assetPath, data);

  // need to flatten out the key value map
  struct KeyValue {
    int key;
    int value;
  };
  std::vector<KeyValue> keyValueVector;
  for (auto keyValue : data.animationKeywordToFrame) {
    for (auto value : keyValue.second) {
      keyValueVector.emplace_back(KeyValue{keyValue.first, value});
    }
  }

  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::ANIM;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  std::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string();
  request.outPath = outputPath.c_str();

  // need to merge the data, name and poses
  std::vector<char> outputData;
  const int nameSize = static_cast<int>(data.name.size() + 1);
  const int posesSize =
      static_cast<int>(data.poses.size() * sizeof(SirEngine::JointPose));
  const int keyValueSize =
      static_cast<int>(sizeof(KeyValue) * keyValueVector.size());
  const int totalSize = nameSize + posesSize + keyValueSize;
  outputData.resize(totalSize);

  // copying the data over
  memcpy(outputData.data(), data.name.data(), nameSize);
  memcpy(outputData.data() + nameSize, data.poses.data(), posesSize);
  memcpy(outputData.data() + nameSize + posesSize, keyValueVector.data(),
         keyValueSize);

  request.bulkData = outputData.data();
  request.bulkDataSizeInByte = totalSize;

  ClipMapperData mapperData;
  mapperData.nameSizeInByte = static_cast<int>(data.name.size() + 1);
  mapperData.posesSizeInByte =
      static_cast<int>(data.poses.size()) * sizeof(SirEngine::JointPose);
  mapperData.frameRate = data.frameRate;
  mapperData.bonesPerFrame = data.bonesPerFrame;
  mapperData.frameCount = data.frameCount;
  mapperData.isLoopable = data.isLoopable;
  mapperData.keyValueSizeInByte = keyValueSize;
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
