#include <SirEngine.h>

class Sandbox : public SirEngine::Application {
public:
  Sandbox() = default;
  ~Sandbox() = default;
};


SirEngine::Application* SirEngine::createApplication()
{
	return new Sandbox();
}