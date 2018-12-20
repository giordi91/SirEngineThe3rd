#pragma once
#include "core.h"
#include "Window.h"


namespace SirEngine
{
	class SIR_ENGINE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void run();

	private:
		std::unique_ptr<Window>m_window;
	};

	//To be implemented by the client
	Application* createApplication();
}

