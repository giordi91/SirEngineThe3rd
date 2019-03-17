#pragma once
#include "nlohmann/json.hpp"
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
  auto dir_it = std::experimental::filesystem::directory_iterator(program_p);
  for (auto p : dir_it) {
    bool is_dir = std::experimental::filesystem::is_directory(p);
    if (!is_dir) {
      auto path = std::experimental::filesystem::path(p);

      if (shouldFilter && !(path.extension() == _extension)) {
        continue;
      }
      auto f_path = std::string(path.native().begin(), path.native().end());
      filePaths.push_back(f_path);
    }
  }
}
inline std::string getFileName(const std::string &path) {
  auto exp_path = std::experimental::filesystem::path(path);
  return exp_path.stem().string();
}
inline std::string getFileExtension(const std::string &path) {
  auto exp_path = std::experimental::filesystem::path(path);
  return exp_path.extension().string();
}

inline std::string getPathName(const std::string &path) {
  auto exp_path = std::experimental::filesystem::path(path);
  return exp_path.parent_path().string();
}

inline bool fileExists(const std::string &name) {
  return std::experimental::filesystem::exists(name);
}
inline bool filePathExists(const std::string &name) {
  std::experimental::filesystem::path path(name);
  std::experimental::filesystem::path parent = path.parent_path();
  return std::experimental::filesystem::exists(parent);
}

template <typename T>
inline T getValueIfInJson(const nlohmann::json &data, const std::string &key,
                   const T &default_value) {
  if (data.find(key) != data.end()) {
    return data[key].get<T>();
  }
  return default_value;
}

#if GRAPHICS_API == DX12
template <>
inline DirectX::XMFLOAT4 getValueIfInJson(const nlohmann::json &data, const std::string& key,
                                   const DirectX::XMFLOAT4 &defValue) {
  if (data.find(key) != data.end()) {
    auto &vec = data[key];
    return DirectX::XMFLOAT4(vec[0].get<float>(), vec[1].get<float>(),
                             vec[2].get<float>(), vec[3].get<float>());
  }
  return defValue;
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
      nlohmann::json j_obj = nlohmann::json::parse(sBuffStr);
      return j_obj;
    } catch (...) {
      // if not lets throw an error
      std::cout << "ERROR, in parsing json file at path: \n"
                << path << std::endl;
      auto ex = std::current_exception();
      ex._RethrowException();
    }
  } else {
    assert(0);
    return nlohmann::json();
  }
}
