#pragma once
#ifndef CLIENT
#include "../Server/Game.h"
typedef struct _Client {
	Game* game;
	HANDLE hPipe;
	HWND hwnd;
}ClientData;
#endif // !CLIENT
