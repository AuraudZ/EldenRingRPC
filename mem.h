#pragma once
#include <Windows.h>
#include <TlHelp32.h>

#include <iostream>
#include <regex>
#include <Psapi.h>

#define PAD(size) struct { unsigned char _padding_##size[size]; }
#define OFFSET( type, func, offset ) type& func { return *reinterpret_cast<type*>( reinterpret_cast<uintptr_t>( this ) + offset ); }  // NOLINT


class Vector3
{
	public:
	float x, y, z;
};


inline void Patch(void* dst, BYTE* src, size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(dst, src, size);
	VirtualProtect(dst, size, oldProtect, NULL);
}

inline void Nop(BYTE* dst, size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memset(dst, 0x90, size);
	VirtualProtect(dst, size, oldProtect, NULL);
}


inline void Parse(char* combo, char* pattern, char* mask)
{
	unsigned int patternLen = (strlen(combo) + 1) / 3;

	for (unsigned int i = 0; i < strlen(combo); i++)
	{
		if (combo[i] == ' ')
		{
			continue;
		}

		else if (combo[i] == '?')
		{
			mask[(i + 1) / 3] = '?';
			i += 2;
		}

		else
		{
			char byte = (char)strtol(&combo[i], 0, 16);
			pattern[(i + 1) / 3] = byte;
			mask[(i + 1) / 3] = 'x';
			i += 2;
		}
	}
	pattern[patternLen] = '\0';
	mask[patternLen] = '\0';
}

MODULEINFO GetModuleInfo(const wchar_t* module) {
	MODULEINFO modInfo = { 0 };
	HMODULE hModule = GetModuleHandleW(module);
	if (hModule == 0) {
		return modInfo;
	}
	GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));
	return modInfo;
}

const char* ReadString(uintptr_t addr, int length) {
	char* TempName = nullptr;
	DWORD64 TempData = *(DWORD64*)addr;
	TempName = reinterpret_cast<char*>(TempData);
	return TempName;
}

char* Scan(char* pattern, char* mask, char* begin, unsigned int size)
{
	unsigned int patternLength = strlen(pattern);

	for (unsigned int i = 0; i < size - patternLength; i++)
	{
		bool found = true;
		for (unsigned int j = 0; j < patternLength; j++)
		{
			if (mask[j] != '?' && pattern[j] != *(begin + i + j))
			{
				found = false;
				break;
			}
		}
		if (found)
		{
			return (begin + i);
		}
	}
	return nullptr;
}

uintptr_t FindPattern(HMODULE hModule, BYTE* pattern, BYTE* mask, size_t patternLength, size_t offsetFromBegin)
{
	MODULEINFO modInfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));

	uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
	size_t size = modInfo.SizeOfImage;

	for (uintptr_t i = 0; i < size - patternLength; ++i)
	{
		bool found = true;
		for (size_t j = 0; j < patternLength; ++j)
		{
			found &= mask[j] == '?' || pattern[j] == *(BYTE*)(base + i + j);
		}

		if (found)
		{
			return base + i + offsetFromBegin;
		}
	}
	return NULL;
}

DWORD64 FindPattern(const wchar_t* modName, const char* signature)
{
	static auto pattern_to_byte = [](const char* pattern)
		{
			auto bytes = std::vector<char>{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + strlen(pattern);

			for (auto current = start; current < end; ++current)
			{
				if (*current == '?')
				{
					++current;
					if (*current == '?')
						++current;
					bytes.push_back('\?');
				}
				else
				{
					bytes.push_back(strtoul(current, &current, 16));
				}
			}
			return bytes;
		};

	MODULEINFO mInfo = GetModuleInfo(modName);
	DWORD64 base = (DWORD64)mInfo.lpBaseOfDll;
	DWORD64 sizeOfImage = (DWORD64)mInfo.SizeOfImage;
	auto patternBytes = pattern_to_byte(signature);

	DWORD64 patternLength = patternBytes.size();
	auto data = patternBytes.data();

	for (DWORD64 i = 0; i < sizeOfImage - patternLength; i++)
	{
		bool found = true;
		for (DWORD64 j = 0; j < patternLength; j++)
		{
			char a = '\?';
			char b = *(char*)(base + i + j);
			found &= data[j] == a || data[j] == b;
		}
		if (found)
		{
			return base + i;
		}
	}
	return NULL;
}


int Add(uintptr_t addr, int value)
{
	int add = *(int*)addr;
	*(int*)add += value;
	return add;
}

void Sub(uintptr_t addr, int value)
{
	*(int*)addr -= value;
}


 
