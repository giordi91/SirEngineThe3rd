#include "editorPath.h"

#include <string>

#include "SirEngine/io/fileUtils.h"
#include "SirEngine/runtimeString.h"

// nasty windows includes for pop up
#include <windows.h>
// needs to happen after windows.h
#include <commdlg.h>
#include <tchar.h>

namespace SirEngine::Editor::io {
constexpr char* RECENT_FILE_NAME = "recent.json";
constexpr char* PROJECT_START_PATH = "";
constexpr wchar_t* WPROJECT_FILE_FILTER = L"Project\0*.PROJECT\0";

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
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (GetOpenFileName(&ofn) == TRUE) {
    return std::wstring(ofn.lpstrFile);
  }
  return std::wstring(L"");
}

const char* loadProjectPathFromRecentFile() { return nullptr; };

const char* getProjectPath() {
  bool result = fileExists(RECENT_FILE_NAME);
  if (result) {
    return loadProjectPathFromRecentFile();
  }
  std::wstring path =
      nativeOpenFileDialog(PROJECT_START_PATH, WPROJECT_FILE_FILTER);
  return path.empty() ? nullptr : frameConvert(path.c_str());
}

}  // namespace SirEngine::Editor::io