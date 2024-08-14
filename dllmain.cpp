// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include <iostream>
#include "mem.h"
#include "Offsets.h"
#include "discord/discord_rpc.h"
#include <capstone/capstone.h>
#include <chrono>
#define PAT "48 8b 35 ? ? ? ? 48 85 f6 ? ? bb 01 00 00 00 89 5c 24 20 48 8b b6"
void InitDiscord()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize("1264060988030980208", &handlers, 1,"");
}


std::wstring_view ReadString(uintptr_t addr) {
    auto string = reinterpret_cast<wchar_t*>(addr);
    return std::wstring_view(string);
}

void LogMapId(uint32_t mapId) {
    Position pos;
    pos.Area = (byte)(mapId >> 24 & 0xFF);
    pos.Block = (byte)(mapId >> 16 & 0xFF);
    pos.Region = (byte)(mapId >> 8 & 0xFF);
    pos.Size = (byte)(mapId & 0xFF);

    printf("Position: %s\n", pos.ToString().c_str());
}

std::string MapIdToName(uint32_t mapId) {
	Position pos;
	pos.Area = (byte)(mapId >> 24 & 0xFF);
	pos.Block = (byte)(mapId >> 16 & 0xFF);
	pos.Region = (byte)(mapId >> 8 & 0xFF);
	pos.Size = (byte)(mapId & 0xFF);

	
    //m10_00_00_00: Stormveil Castle
    std::string identifier = "m" + std::to_string(pos.Area) + "_" + std::to_string(pos.Block) + "_" + std::to_string(pos.Region) + "_" + std::to_string(pos.Size);

    if (identifier == "m10_0_0_0") {
		return "Stormveil Castle";
	}

    if (identifier == "m255_255_255_255") {        return "Loading...";
    }
	
    return identifier;
}

DiscordRichPresence discordPresence;

void UpdatePresence(std::string name, uintptr_t player_ptr) 
{
    char buffer2[256];
    int level = *(int*)(player_ptr + 0x68);
    auto playerName = ReadString(player_ptr + 0x9c);
    if(!playerName.size()) playerName = L"Unknown";
    sprintf_s(buffer2, "%ws - Level: %d", playerName.data(),level);
    discordPresence.state = buffer2;
    discordPresence.details = name.c_str();
    discordPresence.largeImageKey = "logo";
    discordPresence.instance = 1;
    Discord_UpdatePresence(&discordPresence);
}


uintptr_t FindRelativeAddress(const wchar_t* mod_name, const char* pattern) {
    auto patternAddr = FindPattern(mod_name, pattern);
    if (!patternAddr) return 0;

    csh handle;
    cs_insn* insn;

    auto status = cs_open(CS_ARCH_X86, CS_MODE_64, &handle);

    if (status != CS_ERR_OK) {
        printf("Failed to open capstone\n");
        return 0;
    }

    auto asmc = cs_disasm(handle, (uint8_t*)patternAddr, 0x100, patternAddr, 1, &insn);
    uint64_t addr = 0;
    if (asmc > 0) {
        auto size = insn->size;
        auto disp = insn->detail->x86.disp;
        auto add = patternAddr + size + disp;
        return add;
    }
}

void GetLocationData(uintptr_t base,uintptr_t player_ptr) {
	auto WorldChrMan = *(uintptr_t*)(base + 0x3D65FA8);
	if (!WorldChrMan) return;
	auto playerInst = *(uintptr_t*)(WorldChrMan + 0x1e508);
	if (!playerInst) return;
	auto mapId = *(uint32_t*)(playerInst + 0x6d0);
	auto pos = *(Vector3*)(playerInst + 0x6c0);
	auto mapName = MapIdToName(mapId);
	UpdatePresence(mapName,player_ptr);

}



void MainThread(HMODULE hModule)
{
	/*AllocConsole();
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	printf("Hello World\n");*/

    while(!GetModuleHandle("eldenring.exe"))
	{
		Sleep(1000);
	}

    InitDiscord();
    
    auto base = (uintptr_t)GetModuleHandle("eldenring.exe");
    while (!base) {
        base = (uintptr_t)GetModuleHandle("eldenring.exe");
        
    }
    auto start =  FindPattern(L"eldenring.exe", PAT);
    auto gameDataManAOB = FindPattern(L"eldenring.exe", "48 8B 05 ? ? ? ? 48 85 C0 74 05 48 8B 40 58 C3 C3");

    while (!start || !gameDataManAOB) {
        start = FindPattern(L"eldenring.exe", PAT);
        gameDataManAOB = FindPattern(L"eldenring.exe", "48 8B 05 ? ? ? ? 48 85 C0 74 05 48 8B 40 58 C3 C3");
    }
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.startTimestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

   csh handle;
   cs_insn* insn;
   auto status = cs_open(CS_ARCH_X86, CS_MODE_64, &handle);

   if (status != CS_ERR_OK) {
        FreeLibraryAndExitThread(hModule, 0);
		return;
	}


   cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
   
   auto x = cs_disasm(handle, (uint8_t*)start, 0x100, start, 1, &insn);
   uint64_t addr = 0;
   if (x > 0) {
       auto size = insn->size;
       auto disp = insn->detail->x86.disp;
       auto add = start + size + disp;
       auto op_count = insn->detail->x86.op_count;
       for (int i = 0; i < op_count; i++) {
           auto op = insn->detail->x86.operands[i];
           if (op.type == X86_OP_MEM) {
               auto disp = op.mem.disp;
               addr = start + insn->size + disp;
           }
       }
   }


   uint64_t gameDataMan = 0;
   if (gameDataManAOB) {
       auto gameDataOp = cs_disasm(handle, (uint8_t*)gameDataManAOB, 0x100, gameDataManAOB, 1, &insn);
       if (gameDataOp > 0) {
           auto size = insn->size;
           auto disp = insn->detail->x86.disp;
           auto add = gameDataManAOB + size + disp;
           gameDataMan = add;
       }
   }
   if(!gameDataMan) return;
   auto gameDataPtr = *(uintptr_t*)(gameDataMan);
   auto playerPtr = *(uintptr_t*)(gameDataPtr + 0x8);
   auto WorldChrMan = *(uintptr_t*)(addr);   
   UpdatePresence("Starting...",playerPtr);
    while(true)
	{
		Sleep(100);
		GetLocationData(base,playerPtr);
        if(GetAsyncKeyState(VK_DELETE))
		{
			break;
		}
        if (!GetModuleHandle("eldenring.exe")) {
            Discord_Shutdown();
			FreeLibraryAndExitThread(hModule, 0);
        }
	}
    Discord_Shutdown();
	FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

