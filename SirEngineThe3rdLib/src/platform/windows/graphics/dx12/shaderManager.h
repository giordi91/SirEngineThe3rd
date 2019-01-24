#pragma once
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>
#include <d3dcommon.h>

namespace temp{
namespace rendering {

class ShaderManager 
{

public:
  ~ShaderManager() = default;
  // right now this is empty, is kept here for the time being
  // just for simmetry with the other managers
  void init(){};
  void loadShadersInFolder(const char *directory);
  void cleanup();
  inline ID3DBlob *getShaderFromName(const std::string &name) {
    auto found = m_stringToShader.find(name);
    if (found != m_stringToShader.end()) {
      return found->second;
    }
    assert(0 && "could not find shader");
    return nullptr;
  }

  ShaderManager() = default;
  ShaderManager(const ShaderManager &) = delete;
  ShaderManager &operator=(const ShaderManager &) = delete;
  void loadShaderFile(const char *path);

private:
  // data caching
  std::unordered_map<std::string, ID3DBlob *> m_stringToShader;
};
} // namespace rendering
} // namespace dx12
