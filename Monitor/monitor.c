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
	Game game[BUFFERSIZE];
}SharedMem;

typedef struct _ControlData {
	unsigned int shutdown; // Release
	unsigned int id; // Id from the process
	unsigned int count; // Counter for the items
	HANDLE hMapFile; // Memory
	SharedMem* sharedMem; // Shared memory of the game
	HANDLE hMutex; // Mutex
	HANDLE hWriteSem; // Light warns writting
	HANDLE hReadSem; // Light warns reading 
	HANDLE hThreadTime;
	Game* game;
}ControlData;

void showBoard(ControlData* data) {
	WaitForSingleObject(data->hMutex, INFINITE);
	for (DWORD i = 0; i < data->game->rows; i++)
	{
		
		_tprintf(TEXT("\n"));
		for (DWORD j = 0; j < data->game->columns; j++)
			_tprintf(TEXT("%c "), data->game->board[i * data->game->rows + j]);
	}
	_tprintf(TEXT("\n"));
	ReleaseMutex(data->hMutex);
}


DWORD WINAPI receiveData(LPVOID p) {
	ControlData* data = (ControlData*)p;
	Game aux;
	int i = 0;

	while (1) {
		if (data->shutdown == 1)
			return 0;
		WaitForSingleObject(data->hReadSem, INFINITE);
		WaitForSingleObject(data->hMutex, INFINITE);
		CopyMemory(data->game, &(data->sharedMem->game[i]), sizeof(Game));
		i++;
		if (i == BUFFERSIZE)
			i = 0;
		ReleaseMutex(data->hMutex);
		ReleaseSemaphore(data->hWriteSem, 1, NULL);

	}
}


BOOL initMemAndSync(ControlData* p) {
	BOOL firstProcess = FALSE;
	_tprintf(TEXT("\n\nConfigs for the game initializing...\n"));

	p->hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);

	if (p->hMapFile == NULL) { // Map
		firstProcess = TRUE;
		p->hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMem), SHM_NAME);

		if (p->hMapFile == NULL) {
			_tprintf(TEXT("\nErro CreateFileMapping (%d).\n"), GetLastError());
			return FALSE;
		}
	}

	p->sharedMem = (SharedMem*)MapViewOfFile(p->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMem)); // Shared Memory
	if (p->sharedMem == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d)."), GetLastError());
		CloseHandle(p->hMapFile);
		return FALSE;
	}

	p->hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	if (p->hMutex == NULL) {
		_tprintf(TEXT("\nError creating mutex (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		return FALSE;
	}

	p->hWriteSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_WRITE_NAME);
	if (p->hWriteSem == NULL) {
		_tprintf(TEXT("\nError creating writting semaphore: (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		return FALSE;
	}

	p->hReadSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_READ_NAME);
	if (p->hReadSem == NULL) {
		_tprintf(TEXT("\nError creating reading semaphore (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		return FALSE;
	}

	return TRUE;
}





int _tmain(int argc, TCHAR** argv) {
#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif
	Game game;
	ControlData controlData;
	controlData.game = &game;
	TCHAR option[BUFFER] = TEXT(" ");
	HANDLE hThreadReceiveDataFromServer;
	controlData.shutdown = 0;
	controlData.count = 0;

	if (!initMemAndSync(&controlData)) {
		_tprintf(_T("Error creating/opening shared memory and synchronization mechanisms.\n"));
		exit(1);
	}

	hThreadReceiveDataFromServer = CreateThread(NULL, 0, receiveData, &controlData, 0, NULL);
	if (hThreadReceiveDataFromServer == NULL) {
		_tprintf(TEXT("\nCouldnt create thread to receive data from the server.\n"));
		exit(1);
	}

	_tprintf(TEXT("Type in 'exit' to leave.\n"));
	do {
		_getts_s(option, _countof(option)); 
		if (_ttoi(option) == 1)
			showBoard(&controlData);
	} 
	while (_tcscmp(option, TEXT("exit")) != 0);
	controlData.shutdown = 1;
	showBoard(&controlData);
	WaitForSingleObject(hThreadReceiveDataFromServer, INFINITE);

	// Closing of all the handles
	UnmapViewOfFile(controlData.sharedMem);
	CloseHandle(controlData.hMapFile);
	CloseHandle(controlData.hMutex);
	CloseHandle(controlData.hWriteSem);
	CloseHandle(controlData.hReadSem);

	return 0;
}