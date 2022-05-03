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
#define EVENT_NAME TEXT("COMMANDEVENT") //Name of the command event
#define COMMAND_MUTEX_NAME TEXT("COMMANDMUTEX") //Name of the command mutex
#define BUFFERSIZE 10

typedef struct _Game{
	TCHAR board[400];
	DWORD rows;
	DWORD columns;
	DWORD time;
	TCHAR pieces[6];
}Game;

typedef struct _Registry{
	HKEY key;
	TCHAR keyCompletePath[BUFFER];
	DWORD dposition;
	TCHAR name[BUFFER];
}Registry;

typedef struct _SharedMem {
	Game game[BUFFERSIZE];
	DWORD commandMonitor;
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
	HANDLE commandEvent; //event used to coordinate commands received
	HANDLE commandMutex; //mutex used to coordinate commands
	Game* game;
}ControlData;


DWORD WINAPI sendData(LPVOID p) {
	ControlData* data = (ControlData*)p;
	int i = 0;

	while (1) {
		if (data->shutdown == 1)
			return 0;
		WaitForSingleObject(data->hWriteSem, INFINITE);
		WaitForSingleObject(data->hMutex, INFINITE);
		CopyMemory(&(data->sharedMem->game[i]), data->game, sizeof(Game));
		i++;
		if (i == BUFFERSIZE)
			i = 0;
		ReleaseMutex(data->hMutex);
		ReleaseSemaphore(data->hReadSem, 1, NULL);
	}
}


DWORD WINAPI receiveCommnadsMonitor(LPVOID p) {
	ControlData* data = (ControlData*)p;
	DWORD command;

	do {
		WaitForSingleObject(data->commandEvent, INFINITE);
		WaitForSingleObject(data->commandMutex, INFINITE);
		CopyMemory(&command, &(data->sharedMem->commandMonitor), sizeof(DWORD));
		ReleaseMutex(data->commandMutex);

		if (command == 1) {
			_tprintf(TEXT("faucet"));
		}
		if (command == 2) {
			_tprintf(TEXT("insert"));
		}
		if (command == 3) {
			_tprintf(TEXT("random"));
		}
		Sleep(1000);
	} while (data->shutdown != 1);

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

	p->commandEvent = CreateEvent(NULL, TRUE, FALSE, EVENT_NAME);
	if (p->hReadSem == NULL) {
		_tprintf(TEXT("\nError creating reading semaphore (%d).\n"), GetLastError());
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		return FALSE;
	}

	p->commandMutex = CreateMutex(NULL, FALSE, COMMAND_MUTEX_NAME);
	if (p->commandMutex == NULL) {
		_tprintf("\nError creating command mutex");
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		CloseHandle(p->commandEvent);
	}

	return TRUE;
}



BOOL configGame(Registry* registry, ControlData* controlData) {
	// Verifies if the key exists
	if (RegOpenKeyEx(HKEY_CURRENT_USER, registry->keyCompletePath, 0, KEY_ALL_ACCESS, &registry->key) != ERROR_SUCCESS) {
		// If doesnt exists creates the key
		if (RegCreateKeyEx(HKEY_CURRENT_USER, registry->keyCompletePath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &registry->key, &registry->dposition) == ERROR_SUCCESS)
		{
			_tprintf_s(TEXT("\nI just created the key for the PipeGame.\n"));
			DWORD rows = 10;
			DWORD columns = 10;
			DWORD time = 30;

			// Creates chain of values
			long rowsSet = RegSetValueEx(registry->key, TEXT("Rows"), 0, REG_DWORD, (LPBYTE)&rows, sizeof(DWORD));
			long columnsSet = RegSetValueEx(registry->key, TEXT("Columns"), 0, REG_DWORD, (LPBYTE)&columns, sizeof(DWORD));
			long timeSet = RegSetValueEx(registry->key, TEXT("Time"), 0, REG_DWORD, (LPBYTE)&time, sizeof(DWORD));

			if (rowsSet != ERROR_SUCCESS || columnsSet != ERROR_SUCCESS || timeSet != ERROR_SUCCESS) {
				_tprintf_s(TEXT("\nCouldnt configure the values for the pipe game."));
				return FALSE;
			}
		}
	}
	DWORD sizeDword = SIZE_DWORD;
	// Querys the values for the variables
	long rowsRead = RegQueryValueEx(registry->key, TEXT("Rows"), NULL, NULL, (LPBYTE)&controlData->game->rows, &sizeDword);
	long columnsRead = RegQueryValueEx(registry->key, TEXT("Columns"), NULL, NULL, (LPBYTE)&controlData->game->columns, &sizeDword);
	long timeRead = RegQueryValueEx(registry->key, TEXT("Time"), NULL, NULL, (LPBYTE)&controlData->game->time, &sizeDword);

	

	if (rowsRead != ERROR_SUCCESS || columnsRead != ERROR_SUCCESS || timeRead != ERROR_SUCCESS) {
		_tprintf(TEXT("\nCouldnt read the values for the pipe game."));
		return FALSE;
	}
	_tprintf(TEXT("\nBoard Loaded with [%d] rows and [%d] columns, water [%d] seconds.\n"), controlData->game->rows, controlData->game->columns, controlData->game->time);
	return TRUE;
}

int initBoard(ControlData* data) {

	for (DWORD i = 0; i < data->game->rows * data->game->columns; i++) {
		_tcscpy_s(&data->game->board[i], sizeof(TCHAR), TEXT("_"));
	}

	return 1;
}

void startGame(ControlData* data) {
	DWORD randomRow = -1;
	DWORD randomColumn = -1;
	int number;
	srand((unsigned int)time(NULL));
	int quadrante = 0;

	_tcscpy_s(&data->game->pieces[0], sizeof(TCHAR), TEXT("━"));
	_tcscpy_s(&data->game->pieces[1], sizeof(TCHAR), TEXT("┃"));
	_tcscpy_s(&data->game->pieces[2], sizeof(TCHAR), TEXT("┏"));
	_tcscpy_s(&data->game->pieces[3], sizeof(TCHAR), TEXT("┓"));
	_tcscpy_s(&data->game->pieces[4], sizeof(TCHAR), TEXT("┛"));
	_tcscpy_s(&data->game->pieces[5], sizeof(TCHAR), TEXT("┗"));

	number = rand() % 2 + 1;

	if (number == 1) {
		randomRow = rand() % data->game->rows;

		number = rand() % 2 + 1;

		if (number == 1)
			randomColumn = 0;
		else
			randomColumn = data->game->columns - 1;
	}
	else {
		randomColumn = rand() % data->game->columns;

		number = rand() % 2 + 1;

		if (number == 1)
			randomRow = 0;
		else
			randomRow = data->game->rows - 1;
	}
	data->game->board[randomRow * data->game->rows + randomColumn] = 'B';

	if (randomRow < data->game->rows / 2 && randomColumn < data->game->columns / 2)
		quadrante = 1;
	else if (randomRow < data->game->rows / 2 && randomColumn >= data->game->columns / 2)
		quadrante = 2;
	else if (randomRow >= data->game->rows / 2 && randomColumn < data->game->columns / 2)
		quadrante = 3;
	else
		quadrante = 4;

	if (quadrante == 1) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRow = (rand() % (data->game->rows /2) + (data->game->rows /2));
			randomColumn = data->game->columns - 1;
		}
		else {
			randomColumn = (rand() % (data->game->columns/2) + (data->game->columns/2));
			randomRow = data->game->rows - 1;
		}
		data->game->board[randomRow * data->game->rows + randomColumn] = 'E';
	}else if (quadrante == 2) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRow = (rand() % (data->game->rows / 2) + (data->game->rows / 2));
			randomColumn = 0;
		}
		else {
			randomColumn = rand() % (data->game->columns / 2);
			randomRow = data->game->rows-1;
		}
		data->game->board[randomRow * data->game->rows + randomColumn] = 'E';
	}else if (quadrante == 3) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRow = rand() % (data->game->rows / 2);
			randomColumn = data->game->columns-1;
		}
		else {
			randomColumn = (rand() % (data->game->columns / 2) + (data->game->columns/2));
			randomRow = 0;
		}
		data->game->board[randomRow * data->game->rows + randomColumn] = 'E';
	}else if (quadrante == 4) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRow = rand() % (data->game->rows/2);
			randomColumn = 0;
		}
		else {
			randomColumn = rand() % (data->game->columns / 2);
			randomRow = 0;
		}
		data->game->board[randomRow * data->game->rows + randomColumn] = 'E';
	}
}




int _tmain(int argc, TCHAR** argv) {
// Default code for windows32 API
#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

// Variables
	Registry registry;
	Game game;
	ControlData controlData;
	controlData.game = &game;
	_tcscpy_s(registry.keyCompletePath, BUFFER, TEXT("SOFTWARE\\PipeGame\0"));
	TCHAR option[BUFFER] = TEXT(" ");
	HANDLE hThreadSendDataToMonitor;
	HANDLE receiveCommandsMonitorThread;
	controlData.shutdown = 0; // trinco 
	controlData.count = 0; // numero de itens


	_tprintf(TEXT("\n-------------------PIPEGAME---------------\n"));
	if (!initMemAndSync(&controlData)) {
		_tprintf(_T("Error creating/opening shared memory and synchronization mechanisms.\n"));
		exit(1);
	}

	if (argc != 4) {
		if (!configGame(&registry, &controlData)) {
			_tprintf(_T("\n\nError during configuration of the game.\n"));
			exit(1);
		}
	}else {
		controlData.game->rows = _ttoi(argv[1]);
		controlData.game->columns =  _ttoi(argv[2]);
		controlData.game->time =  _ttoi(argv[3]);
	}

	initBoard(&controlData);	
	
	
	// Function to start the game
	startGame(&controlData);
	hThreadSendDataToMonitor = CreateThread(NULL, 0, sendData, &controlData, 0, NULL);
	
	if (hThreadSendDataToMonitor == NULL) {
		_tprintf(TEXT("\nCouldnt create thread to send data to the monitor.\n"));
		exit(1);
	}
	
	receiveCommandsMonitorThread = CreateThread(NULL, 0, receiveCommnadsMonitor, &controlData, 0, NULL);
	if (receiveCommandsMonitorThread == NULL) {
		_tprintf(TEXT("\nCouldn't create thread to receive commands from monitor.\n"));
		exit(1);
	}

	while (_ttoi(option) != 4) {
		_tprintf(TEXT("\n1 - List Players(in development)."));
		_tprintf(TEXT("\n2 - Suspend Game(in development)."));
		_tprintf(TEXT("\n3 - Resume Game(in development)."));
		_tprintf(TEXT("\n4 - Quit\n\n\nCommand: "));

		_fgetts(option, BUFFER, stdin);

		switch (_ttoi(option)) {
		case 4:
			_tprintf(TEXT("\nClosing the application...\n"));
			controlData.shutdown = 1;
			break;
		default: 
			_tprintf(TEXT("\nCouldn´t recognize the command.\n"));
			break;
		}
	}
	
	

	// Closing of all the handles
	RegCloseKey(registry.key);
	UnmapViewOfFile(controlData.sharedMem);
	CloseHandle(controlData.hMapFile);
	CloseHandle(controlData.hMutex);
	CloseHandle(controlData.hWriteSem);
	CloseHandle(controlData.hReadSem);

	return 0;
}