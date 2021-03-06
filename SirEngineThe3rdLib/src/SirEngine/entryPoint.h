#pragma once

#ifdef SE_PLATFORM_WINDOWS
#ifdef SE_DEFINE_ENTRY_POINT
extern SirEngine::Application* SirEngine::createApplication();

int main()
{
	SirEngine::Log::init();

	auto* app= SirEngine::createApplication();
	app->run();
	delete app;
}
#endif

#endif
