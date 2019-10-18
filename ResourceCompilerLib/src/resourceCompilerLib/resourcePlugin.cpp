#include "resourcePlugin.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include <cassert>

const std::string SHARED_LIBRARY_EXTENSION = "dll";

PluginRegistry *PluginRegistry::registryInst = nullptr;

void PluginRegistry::loadPlugin(const std::string &dllPath, bool verbose) {
  auto ws = std::wstring(dllPath.begin(), dllPath.end());
  HMODULE loadedDLL = LoadLibrary(ws.c_str());
  if (loadedDLL) {
    auto loadFun = (RegisterPluginFunction)GetProcAddress(
        loadedDLL, "pluginRegisterFunction");

    if (loadFun) {
      bool res = loadFun(PluginRegistry::getInstance());
      if (res) {
        m_dlls.push_back(loadedDLL);
        if (verbose) {
          SE_CORE_INFO("Successfully loaded plug-in {0}", getFileName(dllPath));
        }
        return;
      } else {
        SE_CORE_ERROR("Problem in loading plug-in {0}", getFileName(dllPath));
      }
    }
    FreeLibrary(loadedDLL);
  } else {
    SE_CORE_ERROR("Could not load dll: {0}", dllPath);
    // Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);
    SE_CORE_ERROR("DLL load error: {0}", message);

    // Free the buffer.
    LocalFree(messageBuffer);
  }
}

void PluginRegistry::loadPluginsInFolder(const std::string &sourcePath,
                                         bool verbose) {
  std::vector<std::string> paths;
  listFilesInFolder(sourcePath.c_str(), paths, SHARED_LIBRARY_EXTENSION);
  for (const auto &path : paths) {
    if (verbose) {
      SE_CORE_INFO("Found plug-in: {0}", getFileName(path));
    }
    loadPlugin(path, verbose);
  }
}

void PluginRegistry::registerFunction(const std::string &name,
                                      ResourceProcessFunction func) {
  m_registry[name] = func;
}

void PluginRegistry::init() {
  if (registryInst == nullptr) {
    PluginRegistry::registryInst = new PluginRegistry();
  }
}

void PluginRegistry::clear() {
  for (auto handle : m_dlls) {
    FreeLibrary(handle);
  }
  m_dlls.clear();
  m_registry.clear();
}
