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
#define BUFFERSIZE 10

typedef struct _Game {
	TCHAR board[400];
	DWORD rows;
	DWORD columns;
	DWORD time;
	TCHAR pieces[6];
}Game;

typedef struct _Registry {
	HKEY key;
	TCHAR keyCompletePath[BUFFER];
	DWORD dposition;
	TCHAR name[BUFFER];
}Registry;

typedef struct _SharedMem {
	unsigned int p; // contador partilhado com o numero de produtores
	unsigned int c; // contador partilhador com o numero de consumidores
	unsigned int wP; // posi��o do buffer circular para a escrita
	unsigned int rP; // posi��o do buffer circular para a leitura
	Game game[BUFFERSIZE];
}SharedMem;

typedef struct _ControlData {
	unsigned int shutdown; // Release
	unsigned int id; // Id from the process
	HANDLE hMapFile; // Memory
	Game* sharedMem; // Shared memory of the game
	HANDLE hMutex; // Mutex
	HANDLE hWriteSem; // Light warns writting
	HANDLE hReadSem; // Light warns reading 
	HANDLE hThreadTime;
}ControlData;

BOOL initMemAndSync(ControlData* p) {
	BOOL firstProcess = FALSE;
	_tprintf(TEXT("\n\nConfigs for the game initializing...\n"));

	p->hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);

	if (p->hMapFile == NULL) { // Map
		firstProcess = TRUE;
		p->hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Game), SHM_NAME);

		if (p->hMapFile == NULL) {
			_tprintf(TEXT("\nErro CreateFileMapping (%d)\n"), GetLastError());
			return FALSE;
		}
	}

	p->sharedMem = (Game*)MapViewOfFile(p->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Game)); // Shared Memory
	if (p->sharedMem == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d)"), GetLastError());
		CloseHandle(p->hMapFile);
		return FALSE;
	}

	p->hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	if (p->hMutex == NULL) {
		_tprintf(TEXT("\nError creating mutex (%d)\n"), GetLastError());
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		return FALSE;
	}

	DWORD lMaximumSem = 5;

	p->hWriteSem = CreateSemaphore(NULL, lMaximumSem, lMaximumSem, SEM_WRITE_NAME);
	if (p->hWriteSem == NULL) {
		_tprintf(TEXT("\nError creating writting semaphore: (%d)\n"), GetLastError());
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		return FALSE;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("\nCant run two servers at once!\n"));
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		ExitProcess(1);
	}


	p->hReadSem = CreateSemaphore(NULL, lMaximumSem, lMaximumSem, SEM_READ_NAME);
	if (p->hReadSem == NULL) {
		_tprintf(TEXT("\nError creating reading semaphore (%d)\n"), GetLastError());
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		return FALSE;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("\nCant run two servers at once!\n"));
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		ExitProcess(1);
	}
	return TRUE;
}

void showBoard(ControlData* data) {
	for (DWORD i = 0; i < data->sharedMem->rows; i++)
	{
		_tprintf(TEXT("\n"));
		for (DWORD j = 0; j < data->sharedMem->columns; j++)
			_tprintf(TEXT("%c "), data->sharedMem->board[i * data->sharedMem->rows + j]);
	}
	_tprintf(TEXT("\n"));
}



int _tmain(int argc, TCHAR** argv) {
#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif
	ControlData controlData;
	initMemAndSync(&controlData);
	showBoard(&controlData);
	return 0;
}