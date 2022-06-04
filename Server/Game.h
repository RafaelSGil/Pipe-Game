#pragma once
#ifndef GAME
#include <Windows.h>
typedef struct _Game {
	TCHAR board[20][20];
	DWORD rows;
	DWORD columns;
	DWORD time;
	DWORD begginingR;
	DWORD begginingC;
	DWORD endR;
	DWORD endC;
	TCHAR pieces[6];
	DWORD shutdown;
	BOOLEAN random;
	DWORD index;
	DWORD suspended;
	TCHAR piece;
}Game;
#endif // !GAME

