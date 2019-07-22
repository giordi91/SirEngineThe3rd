#pragma once
#include "SirEngine/globals.h"
#include "SirEngine/memory/stringPool.h"
namespace SirEngine
{
	inline const char* persistentString(const char* string)
	{
		return globals::STRING_POOL->allocatePersistent(string);
	}

	
	inline const char* frameConcatenation(const char*first, const char* second)
	{
		return globals::STRING_POOL->concatenateFrame(first,second,"");
		
	}
	

}