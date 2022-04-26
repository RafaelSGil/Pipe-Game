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

typedef struct _Registry {
	HKEY key;
	TCHAR keyCompletePath[BUFFER];
	DWORD dposition;
	TCHAR name[BUFFER];
}Registry;

typedef struct _ControlData {
	unsigned int shutdown; // Release
	unsigned int id; // Id from the process
	HANDLE hMapFile; // Memory
	Game* sharedMem; // Shared memory of the game
	HANDLE hMutex; // Mutext
	HANDLE hWriteSem; // Light warns writting
	HANDLE hReadSem; // Light warns reading 
}ControlData;


int _tmain(int argc, TCHAR** argv) {

	_tprintf(TEXT("IM THE MONITOR"));

	return 0;
}