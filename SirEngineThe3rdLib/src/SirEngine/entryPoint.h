#pragma once
#include <iostream>

#ifdef SE_PLATFORM_WINDOWS

extern SirEngine::Application* SirEngine::createApplication();

int main()
{
	SirEngine::Log::init();

	auto* app= SirEngine::createApplication();
	auto end = std::chrono::high_resolution_clock::now();
	app->run();
	delete app;
}

#endif
