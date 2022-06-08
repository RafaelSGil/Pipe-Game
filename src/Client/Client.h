#pragma once
#ifndef CLIENT
#include "../Server/Game.h"
typedef struct _ThreadPlay {
	Game* game;
	HANDLE hPipe;
}ThreadPlay;
#endif // !CLIENT

