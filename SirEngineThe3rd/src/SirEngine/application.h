#pragma once
#include "core.h"


namespace SirEngine
{
	class SIR_ENGINE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void run();
	};

	//To be implemented by the client
	Application* createApplication();
}

