#pragma once

namespace Editor {
class Project {
 public:
  bool initilize(const char* projectPath);
  void cleanup(){};
private:
	const char* m_projectPath = nullptr;
	const char* m_projectName= nullptr;
};
}  // namespace Editor
