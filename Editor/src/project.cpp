#include "project.h"

#include "SirEngine/globals.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/stringPool.h"
#include "SirEngine/runtimeString.h"

static constexpr char* PROJECT_STRUCTURE_FOLDERS[]{
    "assetsCache",         "assetsCache/meshes",     "assetsCache/textures",
    "assetsCache/shaders", "assetsCache/animations",
};

namespace Editor {
bool validateFolderStructure(const char* projectPath) {
  for (const char* folder : PROJECT_STRUCTURE_FOLDERS) {
    const char* currPath =
        SirEngine::frameConcatenation(projectPath, folder, "/");
    bool result = SirEngine::createDirectory(currPath);
    if (!result) {
      SE_CORE_ERROR("Could not create folder {} in project.", currPath);
      return false;
    }
  }
  return true;
}

bool Project::initilize(const char* projectPath) {
  const std::string& projectName = SirEngine::getFileName(projectPath);
  const std::string& projectDirectory = SirEngine::getPathName(projectPath);
  m_projectName = SirEngine::persistentString(projectName.c_str());
  m_projectPath = SirEngine::persistentString(projectDirectory.c_str());

  bool result = validateFolderStructure(m_projectPath);
  if (!result) {
    return false;
  }
  return true;
}
}  // namespace Editor
