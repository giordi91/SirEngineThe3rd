#include "editorPath.h"

#include <string>

#include "SirEngine/io/fileUtils.h"
#include "SirEngine/runtimeString.h"

// nasty windows includes for pop up
#include <windows.h>
// needs to happen after windows.h
#include <commdlg.h>
#include <tchar.h>

#include <nlohmann/json.hpp>

#include "SirEngine/log.h"

namespace SirEngine::Editor::io {
static constexpr char* RECENT_FILE_NAME = "recent.json";
static constexpr char* RECENT_FILE_START_PATH = "";
static constexpr wchar_t* WPROJECT_FILE_FILTER = L"Project\0*.PROJECT\0";
static constexpr char* RECENT_PROJECT_KEY = "recentProject";
static const std::string DEFAULT_STRING;

struct EditorRecentInfo {
  const char* m_recentProject;
};

std::wstring nativeOpenFileDialog(const std::string& startPath,
                                  const wchar_t* filter) {
  OPENFILENAME ofn;         // common dialog box structure
  TCHAR szFile[260] = {0};  // if using TCHAR macros

  std::wstring startPathw(startPath.begin(), startPath.end());

  // Initialize OPENFILENAME
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = filter == nullptr ? _T("All\0*.*\0") : filter;
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = startPath.empty() ? NULL : startPathw.c_str();
  // this flag is important OFN_NOCHANGEDIR, if no present changing the path
  // will change the current working directory
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

  if (GetOpenFileName(&ofn) == TRUE) {
    return std::wstring(ofn.lpstrFile);
  }
  return std::wstring(L"");
}

void writeEditorRencentInfo(const EditorRecentInfo& info, const char* path) {
  nlohmann::json j = {{RECENT_PROJECT_KEY, info.m_recentProject}};
  writeJsonObj(path, j);
}

const char* getPathFromDialog() {
  std::wstring path =
      nativeOpenFileDialog(RECENT_FILE_START_PATH, WPROJECT_FILE_FILTER);

  const char* result = path.empty() ? nullptr : frameConvert(path.c_str());
  EditorRecentInfo info{};
  info.m_recentProject = result == nullptr ? "" : result;
  writeEditorRencentInfo(info, RECENT_FILE_NAME);
  return result;
}

const char* loadProjectPathFromRecentFile() {
  nlohmann::json jobj;
  getJsonObj(RECENT_FILE_NAME, jobj);
  // we have the data we can return it
  std::string path = getValueIfInJson(jobj, RECENT_PROJECT_KEY, DEFAULT_STRING);
  return path.empty() ? nullptr : frameString(path.c_str());
};

const char* getProjectPath() {
  bool result = fileExists(RECENT_FILE_NAME);
  if (result) {
    return loadProjectPathFromRecentFile();
  }
  return getPathFromDialog();
}

}  // namespace SirEngine::Editor::io