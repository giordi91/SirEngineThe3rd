#include "SirEngine/memory/stringPool.h"

namespace SirEngine {
	const char* StringPool::allocateStatic(const char* string)
	{
		auto length = static_cast<uint32_t>(strlen(string) + 1);
		auto flags = static_cast<uint8_t>(STRING_TYPE::CHAR);
		void* memory = m_pool.allocate(length, flags);
		memcpy(memory, string, length);
		return reinterpret_cast<char*>(memory);
	}

	const wchar_t* StringPool::allocateStatic(const wchar_t* string)
	{
		uint64_t length = wcslen(string) + 1;
		auto flags = static_cast<uint8_t>(STRING_TYPE::WIDECHAR);
		auto actualSize = static_cast<uint32_t>(length * sizeof(wchar_t));
		void* memory = m_pool.allocate(actualSize, flags);
		memcpy(memory, string, actualSize);
		return reinterpret_cast<wchar_t*>(memory);
	}

	const char* StringPool::allocateFrame(const char* string)
	{
		auto length = static_cast<uint32_t>(strlen(string) + 1);
		void* memory = m_stackAllocator.allocate(length);
		memcpy(memory, string, length);
		return reinterpret_cast<const char*>(memory);
	}

	const wchar_t* StringPool::allocateFrame(const wchar_t* string)
	{
		uint64_t length = wcslen(string) + 1;
		auto actualSize = static_cast<uint32_t>(length * sizeof(wchar_t));
		void* memory = m_stackAllocator.allocate(actualSize);
		memcpy(memory, string, actualSize);
		return reinterpret_cast<wchar_t*>(memory);
	}
}  // namespace SirEngine
