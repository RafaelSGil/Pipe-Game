#include <io.h>
#include <Windows.h>
#include <fcntl.h>
#include <stdio.h>	
#include <stdlib.h>
#include <tchar.h>

#define BUFFER 256
#define SIZE_DWORD 257*(sizeof(DWORD))
#define SHM_NAME TEXT("fmMsgSpace") // Name of the shared memory
#define MUTEX_NAME TEXT("fmMutex") // Name of the mutex   
#define SEM_WRITE_NAME TEXT("SEM_WRITE") // Name of the writting lightning 
#define SEM_READ_NAME TEXT("SEM_READ")	// Name of the reading lightning  

typedef struct _Game {
	TCHAR* board;
	DWORD rows;
	DWORD columns;
	DWORD time;
}Game;

typedef struct _ControlData {
	unsigned int shutdown; // Release
	unsigned int id; // Id from the process
	HANDLE hMapFile; // Memory
	Game* sharedMem; // Shared memory of the game
	HANDLE hMutex; // Mutext
	HANDLE hWriteSem; // Light warns writting
	HANDLE hReadSem; // Light warns reading 
}ControlData;

void showBoard(Game* game) {
	for (DWORD i = 0; i < game->rows; i++)
	{
		_tprintf(TEXT("\n"));
		for (DWORD j = 0; j < game->columns; j++)
			_tprintf(TEXT("%c "), game->board[i * game->rows + j]);
	}
	_tprintf(TEXT("\n"));
}



int _tmain(int argc, TCHAR** argv) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	ControlData controlData;
	controlData.sharedMem = (Game*)MapViewOfFile(INVALID_HANDLE_VALUE, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Game));
	showBoard(&controlData.sharedMem);
	return 0;
}