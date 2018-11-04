#pragma once

#ifdef SE_PLATFORM_WINDOWS

extern SirEngine::Application* SirEngine::createApplication();

int main(int argc, char** argv)
{

	SirEngine::Log::init();

	SE_CORE_INFO("Hello from core logger");
	int var = 11;
	SE_ERROR("Hello from client Var{0}",var);

	auto* app= SirEngine::createApplication();
	app->run();
	delete app;
}

#endif
