#pragma once
#include <filesystem>

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
