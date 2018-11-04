#pragma once

#ifdef SE_PLATFORM_WINDOWS

extern SirEngine::Application* SirEngine::createApplication();

int main(int argc, char** argv)
{
	auto* app= SirEngine::createApplication();
	app->run();
	delete app;
}

#endif
