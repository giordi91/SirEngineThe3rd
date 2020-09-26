#pragma once
#undef max
#undef min
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "SirEngine/core.h"
#include "nlohmann/json_fwd.hpp"

namespace SirEngine {
// NOTE: requires c++17 filesystem
void SIR_ENGINE_API listFilesInFolder(const char *folderPath,
                                      std::vector<std::string> &filePaths,
                                      const std::string extension = "NONE");

std::string SIR_ENGINE_API getFileName(const std::string &path);

std::string SIR_ENGINE_API getFileExtension(const std::string &path);

std::string SIR_ENGINE_API getPathName(const std::string &path);
void SIR_ENGINE_API writeTextFileToDisk(const std::string& path, const std::string& content);

bool SIR_ENGINE_API fileExists(const std::string &name);

bool SIR_ENGINE_API filePathExists(const std::string &name);

bool SIR_ENGINE_API isPathDirectory(const std::string &name);

glm::mat4 SIR_ENGINE_API
getValueIfInJson(const nlohmann::json &data, const std::string &key,
                 const glm::mat4 &default_value); 

glm::vec4 SIR_ENGINE_API getValueIfInJson(const nlohmann::json &data,
                                          const std::string &key,
                                          const glm::vec4 &defValue);

glm::vec3 SIR_ENGINE_API getValueIfInJson(const nlohmann::json &data,
                                          const std::string &key,
                                          const glm::vec3 &defValue);

glm::vec2 SIR_ENGINE_API getValueIfInJson(const nlohmann::json &data,
                                          const std::string &key,
                                          const glm::vec2 &defValue);

glm::quat SIR_ENGINE_API getValueIfInJson(const nlohmann::json &data,
                                          const std::string &key,
                                          const glm::quat &defValue);

std::string SIR_ENGINE_API getValueIfInJson(const nlohmann::json &data,
                                            const std::string &key,
                                            const std::string &defValue);

unsigned int SIR_ENGINE_API getValueIfInJson(const nlohmann::json &data,
                                             const std::string &key,
                                             const unsigned int &defValue);

void SIR_ENGINE_API assertInJson(const nlohmann::json &jobj,
                                 const std::string &key);

bool SIR_ENGINE_API inJson(const nlohmann::json &jobj, const std::string &key);

void SIR_ENGINE_API getJsonObj(const std::string &path, nlohmann::json& outJson);
}  // namespace SirEngine
