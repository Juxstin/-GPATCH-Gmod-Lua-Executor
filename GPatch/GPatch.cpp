// GPatch.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Scan.h"
#include "memory.h"
DWORD unprotect(DWORD addr)
{
	BYTE* tAddr = (BYTE*)addr;
	do
	{
		tAddr += 16;
	} while (!(tAddr[0] == 0x55 && tAddr[1] == 0x8B && tAddr[2] == 0xEC));

	DWORD funcSz = tAddr - (BYTE*)addr;

	PVOID nFunc = VirtualAlloc(NULL, funcSz, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (nFunc == NULL)
		return addr;

	memcpy(nFunc, (void*)addr, funcSz);

	BYTE* pos = (BYTE*)nFunc;
	BOOL valid = false;
	do
	{
		if (pos[0] == 0x72 && pos[2] == 0xA1 && pos[7] == 0x8B) {
			*(BYTE*)pos = 0xEB;

			DWORD cByte = (DWORD)nFunc;
			do
			{
				if (*(BYTE*)cByte == 0xE8)
				{
					DWORD oFuncPos = addr + (cByte - (DWORD)nFunc);
					DWORD oFuncAddr = (oFuncPos + *(DWORD*)(oFuncPos + 1)) + 5;

					if (oFuncAddr % 16 == 0)
					{
						DWORD relativeAddr = oFuncAddr - cByte - 5;
						*(DWORD*)(cByte + 1) = relativeAddr;

						cByte += 4;
					}
				}

				cByte += 1;
			} while (cByte - (DWORD)nFunc < funcSz);

			valid = true;
		}
		pos += 1;
	} while ((DWORD)pos < (DWORD)nFunc + funcSz);

	if (!valid)
	{
		VirtualFree(nFunc, funcSz, MEM_RELEASE);
		return addr;
	}

	return (DWORD)nFunc;
}

#define LUA_MULTRET -1

enum LuaInterfaceType
{
	LUAINTERFACE_CLIENT = 0,
	LUAINTERFACE_SERVER = 1,
	LUAINTERFACE_MENU = 2
};

class CLuaShared
{
public:
	virtual void            padding00() = 0;
	virtual void*			padding01() = 0;
	virtual void*			padding02() = 0;
	virtual void*			padding03() = 0;
	virtual void*			padding04() = 0;
	virtual void*			padding05() = 0;
	virtual DWORD			GetLuaInterface(LuaInterfaceType type) = 0;
};

typedef void* (*CreateInterfaceFn)(const char *Name, int *ReturnCode);

// Lua function prototypes.
typedef int(__cdecl *_luaL_loadstring)(void *state, const char *s);
_luaL_loadstring  lua_ls = NULL;

typedef int(__cdecl *_lua_pcall)(void *state, int nargs, int nresults, int errfunc);
_lua_pcall lua_pcall = NULL;


// Get Lua shit
HMODULE luaShared = GetModuleHandleA("lua_shared.dll");
CreateInterfaceFn CreateInterface = (CreateInterfaceFn)GetProcAddress(luaShared, "CreateInterface");
CLuaShared* ILuaShared = (CLuaShared*)CreateInterface("LUASHARED003", NULL);
DWORD CLuaInterface = ILuaShared->GetLuaInterface(LUAINTERFACE_CLIENT);
DWORD pGLuaState = *(DWORD*)(CLuaInterface + 0x4);


DWORD WINAPI input(PVOID lvpParameter)
{
	std::string WholeScript = "";
	HANDLE hPipe;
	char buffer[999999];
	DWORD dwRead;
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\GPatch"),
		PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
		PIPE_WAIT,
		1,
		999999,
		999999,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);
	while (hPipe != INVALID_HANDLE_VALUE)
	{
		if (ConnectNamedPipe(hPipe, NULL) != FALSE)
		{
			while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
			{
				buffer[dwRead] = '\0';
				try {
					try {
						WholeScript = WholeScript + buffer;
					}
					catch (...) {
					}
				}
				catch (std::exception e) {

				}
				catch (...) {

				}
			}
			    lua_ls((void*)pGLuaState, WholeScript.c_str());
				lua_pcall((void*)pGLuaState, 0, 0, 0);

			WholeScript = "";
		}
		DisconnectNamedPipe(hPipe);
	}
}

void meme(void) {
	// Allocate a console and redirect console I/O.  
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)input, NULL, NULL, NULL);
	AllocConsole();
	SetConsoleTitleA("G Fucker by Juxstin");
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);

	// Get all the Lua functions we need.

	if (!luaShared) {
		printf("Failed to find lua_shared.dll.\n");
		return;
	}


	//luaL_loadfile = (_luaL_loadfile)GetProcAddress(luaShared, "luaL_loadfile");
//	if (!luaL_loadfile) {
	//	printf("Failed to find function: luaL_loadfile.\n");
	//	return;
	//}
	lua_pcall = (_lua_pcall)GetProcAddress(luaShared, "lua_pcall");
	if (!lua_pcall) {
		printf("Failed to find function: lua_pcall.\n");
		return;
	}
	lua_ls = (_luaL_loadstring)GetProcAddress(luaShared, "luaL_loadstring");
	if (!lua_ls) {
		printf("Failed to find function: luaL_loadstring.\n");
		return;
	}
	// Do the main shit.
	while (1) {
		// Get input from the user.
		char usrInput[2048] = { 0 };
		fgets(usrInput, 2047, stdin);



		if (!CLuaInterface) {
			printf("Failed to get CLuaInterface.\n");
			continue;
		}
		//DWORD pGLuaState = *(DWORD*)(CLuaInterface + 0x4);
		if (!pGLuaState) {
			printf("Failed to get lua_State.\n");
			continue;
		}

		//if (!*(DWORD*)(CLuaInterface + 0x2C)) {
			// Seperate and validate the user input.
			//char *cmd = strtok(usrInput, " ");
			//if (strcmp(cmd, "-run")) {
				//printf("Unknown command: %s\n", cmd);
				//continue;
			//}
			//char *scrDir = strtok(NULL, "\0");
			//scrDir[strlen(scrDir) - 1] = '\0';

			// Execute the Lua script provided.


		//}

		
	}


}

	/*
	DWORD sv_lua = *(DWORD*)((aobscan::scan("00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\xB4\x1D\x00\x00\x01", "???xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx??x")));
	DWORD sv_luax = unprotect(sv_lua);

	//HMODULE mainaddr = GetModuleHandle(TEXT("server.dll"));
	DWORD address = (DWORD)sv_luax;
	VirtualProtect((LPVOID)address, 1, PAGE_EXECUTE_READWRITE, NULL);
	*(char *)address = 0x01;
	*/