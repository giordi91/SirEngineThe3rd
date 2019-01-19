#pragma once
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <sstream>

nlohmann::json getJsonObj(std::string path) {

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
      SE_CORE_ERROR("Error parsing json file from path {0}", path);
      auto ex = std::current_exception();
      ex._RethrowException();
      return nlohmann::json();
    }
  } else {
    assert(0);
    return nlohmann::json();
  }
}
