#pragma once
#include <stdint.h>
#include "mem.h"

namespace Base {
	int currentLocationPtr = 0x45933B0;
};

struct PlayerGameData {
	PAD(0x9c);
	const char* Name;
};

struct GameData {
	PAD(0x8);
	PlayerGameData PlayerGameData;
};

struct Position {
	byte Area;
	byte Block;
	byte Region;
	byte Size;

	std::string ToString() {
		return "Area: " + std::to_string(Area) + " Block: " + std::to_string(Block) + " Region: " + std::to_string(Region) + " Size: " + std::to_string(Size);
	}
};


