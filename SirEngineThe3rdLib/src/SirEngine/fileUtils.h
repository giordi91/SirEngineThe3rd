#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include "nlohmann/json.hpp"
#include <iostream>

// NOTE: requires c++17 filesystem
inline void listFilesInFolder(const char *folder_path,
                              std::vector<std::string> &file_paths,
                              std::string extension = "NONE") {
  bool should_filter = extension != "NONE";
  std::string _extension = "." + extension;
  auto program_p = std::experimental::filesystem::path(folder_path);
  auto dir_it = std::experimental::filesystem::directory_iterator(program_p);
  for (auto p : dir_it) {
    bool is_dir = std::experimental::filesystem::is_directory(p);
    if (!is_dir) {
      auto path = std::experimental::filesystem::path(p);

      if (should_filter && !(path.extension() == _extension)) {
        continue;
      }
      auto f_path = std::string(path.native().begin(), path.native().end());
      file_paths.push_back(f_path);
    }
  }
}
inline const std::string getFileName(const std::string &path) {
  auto exp_path = std::experimental::filesystem::path(path);
  return exp_path.filename().string();
}

inline bool fileExists(const std::string &name) {
  return std::experimental::filesystem::exists(name);
}
inline bool filePathExists(const std::string &name) {
	std::experimental::filesystem::path path(name);
	std::experimental::filesystem::path parent= path.parent_path();
	return std::experimental::filesystem::exists(parent);
}

template <typename T>
T getValueIfInJson(const nlohmann::json &data, const std::string &key,
                   const T &default_value) {
  if (data.find(key) != data.end()) {
    return data[key].get<T>();
  }
  return default_value;
}

inline nlohmann::json get_json_obj(std::string path) {

  bool res = fileExists(path);
  if (res) {
    // let s open the stream
    std::ifstream st(path);
    std::stringstream s_buffer;
    s_buffer << st.rdbuf();
    std::string s_buff = s_buffer.str();

    try {
      // try to parse
      nlohmann::json j_obj = nlohmann::json::parse(s_buff);
      return j_obj;
    } catch (...) {
      // if not lets throw an error
      std::cout << "ERROR, in parsing json file at path: \n"
                << path << std::endl;
      auto ex = std::current_exception();
      ex._RethrowException();
      return nlohmann::json();
    }
  } else {
    assert(0);
    return nlohmann::json();
  }
}