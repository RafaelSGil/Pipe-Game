#include <io.h>
#include <Windows.h>
#include <fcntl.h>
#include <stdio.h>	
#include <stdlib.h>
#include <tchar.h>

#define BUFFER 256
#define SIZE_DWORD 257*(sizeof(DWORD))
#define SHM_NAME_GAME TEXT("fmGameSpace") // Name of the shared memory for the game
#define SHM_NAME_MESSAGE TEXT("fmMsgSpace") // Name of the shared memory for the message
#define MUTEX_NAME TEXT("fmMutex") // Name of the mutex   
#define MUTEX_NAME_PLAY TEXT("fmMutexPlay") // Name of the mutex   
#define SEM_WRITE_NAME TEXT("SEM_WRITE") // Name of the writting lightning 
#define SEM_READ_NAME TEXT("SEM_READ")	// Name of the reading lightning
#define EVENT_NAME TEXT("COMMANDEVENT") //Name of the command event
#define COMMAND_MUTEX_NAME TEXT("COMMANDMUTEX") //Name of the command mutex
#define BUFFERSIZE 10

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
}Game;

typedef struct _Registry {
	HKEY key;
	TCHAR keyCompletePath[BUFFER];
	DWORD dposition;
	TCHAR name[BUFFER];
}Registry;

typedef struct _SharedMemGame {
	Game game;
}SharedMemGame;

typedef struct _SharedMemCommand {
	DWORD commandMonitor[BUFFERSIZE];
}SharedMemCommand;


typedef struct _ControlData {
	unsigned int id; // Id from the process
	unsigned int count; // Counter for the items
	HANDLE hMapFileGame; // Memory from the game
	HANDLE hMapFileMemory; // Memory from the message
	SharedMemGame* sharedMemGame; // Shared memory of the game
	SharedMemCommand* sharedMemCommand; // Shared memory of the command from the monitor
	HANDLE hMutex; // Mutex
	HANDLE hMutexPlay;
	HANDLE hWriteSem; // Light warns writting
	HANDLE hReadSem; // Light warns reading 
	HANDLE commandEvent; //event used to coordinate commands received
	HANDLE commandMutex; //mutex used to coordinate commands
	Game* game;
}ControlData;

void showBoard(ControlData* data) {
	WaitForSingleObject(data->hMutex, INFINITE);
	for (DWORD i = 0; i < data->game->rows; i++)
	{
		
		_tprintf(TEXT("\n"));
		for (DWORD j = 0; j < data->game->columns; j++)
			_tprintf(TEXT("%c "), data->game->board[i][j]);
	}
	_tprintf(TEXT("\n"));
	ReleaseMutex(data->hMutex);
}


DWORD WINAPI receiveData(LPVOID p) {
	ControlData* data = (ControlData*)p;

	while (1) {
		if (data->game->shutdown == 1)
			return 0;
		WaitForSingleObject(data->hReadSem, INFINITE);
		WaitForSingleObject(data->hMutex, INFINITE);
		CopyMemory(data->game, &(data->sharedMemGame->game), sizeof(Game));
		ReleaseMutex(data->hMutex);
		ReleaseSemaphore(data->hWriteSem, 1, NULL);

	}
}

DWORD WINAPI executeCommands(LPVOID p) {
	ControlData* data = (ControlData*)p;
	TCHAR option[BUFFER] = TEXT(" ");
	DWORD command;
	int i = 0;

	do {
		_getts_s(option, _countof(option));

		if (_tcscmp(option, TEXT("show")) == 0)
			showBoard(data);
		else if (_tcscmp(option, TEXT("faucet")) == 0) {
			WaitForSingleObject(data->commandMutex, INFINITE);
			if (i == BUFFERSIZE)
				i = 0;
			command = 1;
			CopyMemory(&(data->sharedMemCommand->commandMonitor[i]), &command, sizeof(DWORD));
			i++;
			ReleaseMutex(data->commandMutex);
			SetEvent(data->commandEvent);
			ResetEvent(data->commandEvent);
		}else if (_tcscmp(option, TEXT("insert")) == 0) {
			WaitForSingleObject(data->commandMutex, INFINITE);
			if (i == BUFFERSIZE)
				i = 0;
			command = 2;
			CopyMemory(&(data->sharedMemCommand->commandMonitor[i]), &command, sizeof(DWORD));
			i++;
			ReleaseMutex(data->commandMutex);
			SetEvent(data->commandEvent);
			ResetEvent(data->commandEvent);
		}else if (_tcscmp(option, TEXT("random")) == 0) {
			WaitForSingleObject(data->commandMutex, INFINITE);
			if (i == BUFFERSIZE)
				i = 0;
			command = 3;
			CopyMemory(&(data->sharedMemCommand->commandMonitor[i]), &command, sizeof(DWORD));
			i++;
			ReleaseMutex(data->commandMutex);
			SetEvent(data->commandEvent);
			ResetEvent(data->commandEvent);
		}else {
			_tprintf(TEXT("\nCouldnt recognize command.\n"));
		}
	} while (_tcscmp(option, TEXT("exit")) != 0);

	data->game->shutdown = 1;
	return 1;
}


BOOL initMemAndSync(ControlData* p) {
	BOOL firstProcess = FALSE;
	_tprintf(TEXT("\n\nConfigs for the monitor initializing...\n\n"));

	p->hMapFileGame = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME_GAME);

	if (p->hMapFileGame == NULL) { // Map
		firstProcess = TRUE;
		p->hMapFileGame = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemGame), SHM_NAME_GAME);

		if (p->hMapFileGame == NULL) {
			_tprintf(TEXT("\nError CreateFileMapping (%d).\n"), GetLastError());
			return FALSE;
		}
	}

	p->sharedMemGame = (SharedMemGame*)MapViewOfFile(p->hMapFileGame, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemGame)); // Shared Memory
	if (p->sharedMemGame == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d)."), GetLastError());
		CloseHandle(p->hMapFileGame);
		return FALSE;
	}

	p->hMapFileMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME_MESSAGE);
	if (p->hMapFileMemory == NULL) {
		p->hMapFileMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemCommand), SHM_NAME_MESSAGE);

		if (p->hMapFileMemory == NULL) {
			_tprintf(TEXT("\nError CreateFileMapping (%d).\n"), GetLastError());
			return FALSE;
		}
	}

	p->sharedMemCommand = (SharedMemCommand*)MapViewOfFile(p->hMapFileMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemCommand));
	if (p->sharedMemCommand == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMapFileMemory);
		return FALSE;
	}

	p->hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	if (p->hMutex == NULL) {
		_tprintf(TEXT("\nError creating mutex (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMapFileMemory);
		UnmapViewOfFile(p->sharedMemCommand);
		return FALSE;
	}

	p->hWriteSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_WRITE_NAME);
	if (p->hWriteSem == NULL) {
		_tprintf(TEXT("\nError creating writting semaphore: (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hMapFileMemory);
		UnmapViewOfFile(p->sharedMemCommand);
		return FALSE;
	}

	p->hReadSem = CreateSemaphore(NULL, BUFFERSIZE, BUFFERSIZE, SEM_READ_NAME);
	if (p->hReadSem == NULL) {
		_tprintf(TEXT("\nError creating reading semaphore (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hMapFileMemory);
		UnmapViewOfFile(p->sharedMemCommand);
		return FALSE;
	}

	p->commandEvent = CreateEvent(NULL, TRUE, FALSE, EVENT_NAME);
	if (p->commandEvent == NULL) {
		_tprintf(TEXT("\nError creating command event (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		CloseHandle(p->hMapFileMemory);
		UnmapViewOfFile(p->sharedMemCommand);
		return FALSE;
	}

	p->commandMutex = CreateMutex(NULL, FALSE, COMMAND_MUTEX_NAME);
	if (p->commandMutex == NULL) {
		_tprintf(TEXT("\nError creating command mutex.\n"));
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		CloseHandle(p->commandEvent);
		CloseHandle(p->hMapFileMemory);
		UnmapViewOfFile(p->sharedMemCommand);
		return FALSE;
	}

	p->hMutexPlay = CreateMutex(NULL, FALSE, MUTEX_NAME_PLAY);
	if (p->hMutexPlay == NULL) {
		_tprintf(TEXT("\nError creating play mutex.\n"));
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		CloseHandle(p->commandEvent);
		CloseHandle(p->hMapFileMemory);
		UnmapViewOfFile(p->sharedMemCommand);
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
	HANDLE hThreadReceiveDataFromServer;
	HANDLE executeCommandsThread;
	controlData.game->shutdown = 0;
	controlData.count = 0;

	if (OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_WRITE_NAME) == NULL) {
		_tprintf(TEXT("\nCant launch monitor because server isnt running.\n"));
		exit(1);
	}
	if (!initMemAndSync(&controlData)) {
		_tprintf(_T("Error creating/opening shared memory and synchronization mechanisms.\n"));
		exit(1);
	}

	hThreadReceiveDataFromServer = CreateThread(NULL, 0, receiveData, &controlData, 0, NULL);
	if (hThreadReceiveDataFromServer == NULL) {
		_tprintf(TEXT("\nCouldnt create thread to receive data from the server.\n"));
		exit(1);
	}

	executeCommandsThread = CreateThread(NULL, 0, executeCommands, &controlData, 0, NULL);
	if (executeCommandsThread == NULL) {
		_tprintf(TEXT("\nCouldnt create thread to manage commands.\n"));
		exit(1);
	}

	_tprintf(TEXT("Type in 'exit' to leave.\n"));
	while (1) {
		if (controlData.game->shutdown == 1) {
			_tprintf(TEXT("\nShutting down...\n"));
			break;
		}
	}
	//WaitForSingleObject(hThreadReceiveDataFromServer, INFINITE);
	//WaitForSingleObject(executeCommandsThread, INFINITE);

	// Closing of all the handles
	UnmapViewOfFile(controlData.sharedMemCommand);
	UnmapViewOfFile(controlData.sharedMemGame);
	CloseHandle(controlData.hMapFileGame);
	CloseHandle(controlData.hMapFileMemory);
	CloseHandle(controlData.hMutex);
	CloseHandle(controlData.hWriteSem);
	CloseHandle(controlData.hReadSem);

	return 0;
}