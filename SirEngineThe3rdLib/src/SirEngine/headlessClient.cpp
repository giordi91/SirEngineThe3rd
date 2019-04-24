#include "SirEngine/headlessClient.h"
#include "SirEngine/graphics/graphicsCore.h"
#include <cassert>


namespace SirEngine
{
	HeadlessClient::HeadlessClient()
	{
		graphics::initializeGraphics(nullptr,0,0);
	}

	HeadlessClient::~HeadlessClient()
	{
		graphics::shutdownGraphics();
	}

	void HeadlessClient::beginWork()
	{
		assert(0);
	}

	void HeadlessClient::endWork()
	{
		assert(0);
	}

	void HeadlessClient::flushAllOperation()
	{
		assert(0);
	}
}
