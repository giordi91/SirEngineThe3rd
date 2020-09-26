#include "SirEngine/io/fileUtils.h"
#include "nlohmann/json.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace SirEngine {

void listFilesInFolder(const char *folderPath,
                       std::vector<std::string> &filePaths,
                       const std::string extension) {
  bool shouldFilter = extension != "NONE";
  const std::string _extension = "." + extension;
  auto program_p = std::filesystem::path(folderPath);
  auto dirIt = std::filesystem::directory_iterator(program_p);
  for (auto p : dirIt) {
    bool isDir = std::filesystem::is_directory(p);
    if (!isDir) {
      auto path = std::filesystem::path(p);

      if (shouldFilter && !(path.extension() == _extension)) {
        continue;
      }
      filePaths.push_back(path.u8string());
    }
  }
}

std::string getFileName(const std::string &path) {
  const auto expPath = std::filesystem::path(path);
  return expPath.stem().string();
}

std::string getFileExtension(const std::string &path) {
  const auto expPath = std::filesystem::path(path);
  return expPath.extension().string();
}

std::string getPathName(const std::string &path) {
  const auto expPath = std::filesystem::path(path);
  return expPath.parent_path().string();
}

bool fileExists(const std::string &name) {
  return std::filesystem::exists(name);
}

bool filePathExists(const std::string &name) {
  const std::filesystem::path path(name);
  const std::filesystem::path parent = path.parent_path();
  return std::filesystem::exists(parent);
}

bool isPathDirectory(const std::string &name) {
  return std::filesystem::is_directory(name);
}

bool inJson(const nlohmann::json &jobj, const std::string &key) {
  const auto found = jobj.find(key);
  return found != jobj.end();
}

glm::vec4 getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::vec4 &defValue) {
  if (data.find(key) != data.end()) {
    auto &vec = data[key];
    return glm::vec4(vec[0].get<float>(), vec[1].get<float>(),
                     vec[2].get<float>(), vec[3].get<float>());
  }
  return defValue;
}

glm::vec3 getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::vec3 &defValue) {
  if (data.find(key) != data.end()) {
    auto &vec = data[key];
    return glm::vec3(vec[0].get<float>(), vec[1].get<float>(),
                     vec[2].get<float>());
  }
  return defValue;
}

glm::vec2 getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::vec2 &defValue) {
  if (data.find(key) != data.end()) {
    auto &vec = data[key];
    return glm::vec2(vec[0].get<float>(), vec[1].get<float>());
  }
  return defValue;
}

glm::quat getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::quat &defValue) {
  if (data.find(key) != data.end()) {
    auto &vec = data[key];
    // NOTE: glm quaternion wants first the W component then xyz
    return glm::quat(vec[3].get<float>(), vec[0].get<float>(),
                     vec[1].get<float>(), vec[2].get<float>());
  }
  return defValue;
}

std::string getValueIfInJson(const nlohmann::json &data, const std::string &key,
                             const std::string &defValue) {
  if (data.find(key) != data.end()) {
    return data[key].get<std::string>();
  }
  return defValue;
}

unsigned int getValueIfInJson(const nlohmann::json &data,
                              const std::string &key,
                              const unsigned &defValue) {
  if (data.find(key) != data.end()) {
    return data[key].get<unsigned int>();
  }
  return defValue;
}

glm::mat4 SIR_ENGINE_API getValueIfInJson(const nlohmann::json &data,
                                          const std::string &key,
                                          const glm::mat4 &default_value) {
  if (data.find(key) != data.end()) {
    auto &mat = data[key];
    return glm::mat4(
        mat[0].get<float>(), mat[1].get<float>(), mat[2].get<float>(),
        mat[3].get<float>(), mat[4].get<float>(), mat[5].get<float>(),
        mat[6].get<float>(), mat[7].get<float>(), mat[8].get<float>(),
        mat[9].get<float>(), mat[10].get<float>(), mat[11].get<float>(),
        mat[12].get<float>(), mat[13].get<float>(), mat[14].get<float>(),
        mat[15].get<float>());
  }
  return default_value;
}

void assertInJson(const nlohmann::json &jobj, const std::string &key) {
  const auto found = jobj.find(key);
  assert(found != jobj.end());
}

void getJsonObj(const std::string &path, nlohmann::json &outJson) {
  bool res = fileExists(path);
  if (res) {
    // let s open the stream
    std::ifstream st(path);
    std::stringstream sBuffer;
    sBuffer << st.rdbuf();
    std::string sBuffStr = sBuffer.str();

    try {
      // try to parse
      nlohmann::json jObj = nlohmann::json::parse(sBuffStr);
      outJson = jObj;
    } catch (...) {
      // if not lets throw an error
      std::cout << "ERROR, in parsing json file at path: \n"
                << path << std::endl;
      auto ex = std::current_exception();
      std::rethrow_exception(ex);
    }
  } else {
    assert(0);
    outJson = nlohmann::json();
  }
}
}  // namespace SirEngine
