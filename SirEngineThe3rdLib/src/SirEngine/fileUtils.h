#pragma once
#include "nlohmann/json.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#if GRAPHICS_API == DX12
#include <DirectXMath.h>
#endif

// NOTE: requires c++17 filesystem
inline void listFilesInFolder(const char *folderPath,
                              std::vector<std::string> &filePaths,
                              std::string extension = "NONE") {
  bool shouldFilter = extension != "NONE";
  std::string _extension = "." + extension;
  auto program_p = std::experimental::filesystem::path(folderPath);
  auto dirIt = std::experimental::filesystem::directory_iterator(program_p);
  for (auto p : dirIt) {
    bool isDir = std::experimental::filesystem::is_directory(p);
    if (!isDir) {
      auto path = std::experimental::filesystem::path(p);

      if (shouldFilter && !(path.extension() == _extension)) {
        continue;
      }

      // auto fPath = std::string(path.native().begin(), path.native().end());
      filePaths.push_back(path.u8string());
    }
  }
}
inline std::string getFileName(const std::string &path) {
  const auto expPath = std::experimental::filesystem::path(path);
  return expPath.stem().string();
}
inline std::string getFileExtension(const std::string &path) {
  const auto expPath = std::experimental::filesystem::path(path);
  return expPath.extension().string();
}

inline std::string getPathName(const std::string &path) {
  const auto expPath = std::experimental::filesystem::path(path);
  return expPath.parent_path().string();
}

inline bool fileExists(const std::string &name) {
  return std::experimental::filesystem::exists(name);
}
inline bool filePathExists(const std::string &name) {
  const std::experimental::filesystem::path path(name);
  const std::experimental::filesystem::path parent = path.parent_path();
  return std::experimental::filesystem::exists(parent);
}

template <typename T>
inline T getValueIfInJson(const nlohmann::json &data, const std::string &key,
                          const T &defaultValue) {
  if (data.find(key) != data.end()) {
    return data[key].get<T>();
  }
  return defaultValue;
}

#if GRAPHICS_API == DX12
template <>
inline DirectX::XMFLOAT4 getValueIfInJson(const nlohmann::json &data,
                                          const std::string &key,
                                          const DirectX::XMFLOAT4 &defValue) {
  if (data.find(key) != data.end()) {
    auto &vec = data[key];
    return DirectX::XMFLOAT4(vec[0].get<float>(), vec[1].get<float>(),
                             vec[2].get<float>(), vec[3].get<float>());
  }
  return defValue;
}
template <>
inline DirectX::XMVECTOR getValueIfInJson(const nlohmann::json &data,
                                          const std::string &key,
                                          const DirectX::XMVECTOR &defValue) {
  if (data.find(key) != data.end()) {
    auto &vec = data[key];
    return DirectX::XMVectorSet(vec[0].get<float>(), vec[1].get<float>(),
                                vec[2].get<float>(), vec[3].get<float>());
  }
  return defValue;
}
template <>
inline DirectX::XMFLOAT3 getValueIfInJson(const nlohmann::json &data,
                                          const std::string &key,
                                          const DirectX::XMFLOAT3 &defValue) {
  if (data.find(key) != data.end()) {
    auto &vec = data[key];
    return DirectX::XMFLOAT3(vec[0].get<float>(), vec[1].get<float>(),
                             vec[2].get<float>());
  }
  return defValue;
}
template <>
inline DirectX::XMMATRIX
getValueIfInJson(const nlohmann::json &data, const std::string &key,
                 const DirectX::XMMATRIX &default_value) {

  if (data.find(key) != data.end()) {
    auto &mat = data[key];
    return DirectX::XMMATRIX(
        mat[0].get<float>(), mat[1].get<float>(), mat[2].get<float>(),
        mat[3].get<float>(), mat[4].get<float>(), mat[5].get<float>(),
        mat[6].get<float>(), mat[7].get<float>(), mat[8].get<float>(),
        mat[9].get<float>(), mat[10].get<float>(), mat[11].get<float>(),
        mat[12].get<float>(), mat[13].get<float>(), mat[14].get<float>(),
        mat[15].get<float>());
  }
  return default_value;
}
#endif

inline nlohmann::json getJsonObj(std::string path) {

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
      return jObj;
    } catch (...) {
      // if not lets throw an error
      std::cout << "ERROR, in parsing json file at path: \n"
                << path << std::endl;
      auto ex = std::current_exception();
      std::rethrow_exception(ex);
    }
  } else {
    assert(0);
    return nlohmann::json();
  }
}
