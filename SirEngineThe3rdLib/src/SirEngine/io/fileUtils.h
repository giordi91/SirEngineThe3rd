#pragma once
#undef max
#undef min
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "SirEngine/core.h"
#include "nlohmann/json_fwd.hpp"

namespace SirEngine {
// NOTE: requires c++17 filesystem
void listFilesInFolder(const char *folderPath,
                       std::vector<std::string> &filePaths,
                       const std::string extension = "NONE");

std::string getFileName(const std::string &path);

std::string getFileExtension(const std::string &path);

std::string getPathName(const std::string &path);
void writeTextFileToDisk(const std::string &path, const std::string &content);

bool fileExists(const std::string &name);

bool filePathExists(const std::string &name);

bool isPathDirectory(const std::string &name);
bool createDirectory(const std::string &directoryPath);

glm::mat4 getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::mat4 &defaultValue);

glm::vec4 getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::vec4 &defValue);

glm::vec3 getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::vec3 &defValue);

glm::vec2 getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::vec2 &defValue);

glm::quat getValueIfInJson(const nlohmann::json &data, const std::string &key,
                           const glm::quat &defValue);

std::string getValueIfInJson(const nlohmann::json &data, const std::string &key,
                             const std::string &defValue);

uint32_t  getValueIfInJson(const nlohmann::json &data,
                              const std::string &key,
                              const uint32_t &defValue);

int getValueIfInJson(const nlohmann::json &data,
                              const std::string &key,
                              const int &defValue);

void assertInJson(const nlohmann::json &jobj, const std::string &key);

bool inJson(const nlohmann::json &jobj, const std::string &key);

void getJsonObj(const std::string &path, nlohmann::json &outJson);
void writeJsonObj(const char *outPath, nlohmann::json &outJson);
}  // namespace SirEngine
