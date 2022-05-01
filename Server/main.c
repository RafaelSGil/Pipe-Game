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

typedef struct _Game{
	TCHAR* board;
	DWORD rows;
	DWORD columns;
	DWORD time;
	TCHAR* pieces;
	BOOL finished;
}Game;

typedef struct _Registry{
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
	HANDLE hThreadTime;
}ControlData;

LPVOID WINAPI decreaseTime(LPVOID data) {
	Game* aux = (Game*)data;
	while (!aux->finished) {
		aux->time--;
		Sleep(1000);
	}
	ExitThread(1);
}

BOOL initMemAndSync(ControlData* p){
	BOOL firstProcess = FALSE;
	_tprintf(TEXT("\n\nConfigs for the game initializing...\n"));

	if (initBoard(p) == -1) {
		_tprintf(TEXT("\nError initializing the board!\n"));
		return FALSE;
	}

	p->hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);

	if (p->hMapFile == NULL) { // Map
		firstProcess = TRUE;
		p->hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Game), SHM_NAME);

		if (p->hMapFile == NULL) {
			_tprintf(TEXT("\nErro CreateFileMapping (%d)\n"), GetLastError());
			free(p->sharedMem->pieces);
			free(p->sharedMem->board);
			return FALSE;
		}
	}

	p->sharedMem = (Game*)MapViewOfFile(p->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Game)); // Shared Memory
	if (p->sharedMem == NULL) {
		_tprintf(TEXT("\nError: MapViewOfFile (%d)"), GetLastError());
		free(p->sharedMem->pieces);
		free(p->sharedMem->board);
		CloseHandle(p->hMapFile);
		return FALSE;
	}

	p->hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	if (p->hMutex == NULL) {
		_tprintf(TEXT("\nError creating mutex (%d)\n"), GetLastError());
		free(p->sharedMem->board);
		free(p->sharedMem->pieces);
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		return FALSE;
	}

	DWORD lMaximumSem = 1;

	p->hWriteSem = CreateSemaphore(NULL, lMaximumSem, lMaximumSem, SEM_WRITE_NAME);
	if(p->hWriteSem == NULL){
		_tprintf(TEXT("\nError creating writting semaphore: (%d)\n"), GetLastError());
		free(p->sharedMem->board);
		free(p->sharedMem->pieces);
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		return FALSE;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("\nCant run two servers at once!\n"));
		free(p->sharedMem->pieces);
		free(p->sharedMem->board);
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		ExitProcess(1);
	}


	p->hReadSem = CreateSemaphore(NULL, lMaximumSem, lMaximumSem, SEM_READ_NAME);
	if (p->hReadSem == NULL) {
		_tprintf(TEXT("\nError creating reading semaphore (%d)\n"), GetLastError());
		free(p->sharedMem->board);
		free(p->sharedMem->pieces);
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		return FALSE;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("\nCant run two servers at once!\n"));
		free(p->sharedMem->board);
		free(p->sharedMem->pieces);
		UnmapViewOfFile(p->sharedMem);
		CloseHandle(p->hMapFile);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
		ExitProcess(1);
	}
	return TRUE;
}


BOOL configGame(Registry* registry, ControlData* controlData) {
	// Verifies if the key exists
	if (RegOpenKeyEx(HKEY_CURRENT_USER, registry->keyCompletePath, 0, KEY_ALL_ACCESS, &registry->key) != ERROR_SUCCESS) {
		// If doesnt exists creates the key
		if (RegCreateKeyEx(HKEY_CURRENT_USER, registry->keyCompletePath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &registry->key, &registry->dposition) == ERROR_SUCCESS)
		{
			_tprintf_s(TEXT("\nI just created the key for the PipeGame!"));
			DWORD rows = 10;
			DWORD columns = 10;
			DWORD time = 30;

			// Creates chain of values
			long rowsSet = RegSetValueEx(registry->key, TEXT("Rows"), 0, REG_DWORD, (LPBYTE)&rows, sizeof(DWORD));
			long columnsSet = RegSetValueEx(registry->key, TEXT("Columns"), 0, REG_DWORD, (LPBYTE)&columns, sizeof(DWORD));
			long timeSet = RegSetValueEx(registry->key, TEXT("Time"), 0, REG_DWORD, (LPBYTE)&time, sizeof(DWORD));

			if (rowsSet != ERROR_SUCCESS || columnsSet != ERROR_SUCCESS || timeSet != ERROR_SUCCESS) {
				_tprintf_s(TEXT("\nCouldnt configure the values for the pipe game!"));
				return FALSE;
			}
		}
	}
	DWORD sizeDword = SIZE_DWORD;
	// Querys the values for the variables
	long rowsRead = RegQueryValueEx(registry->key, TEXT("Rows"), NULL, NULL, (LPBYTE)&controlData->sharedMem->rows, &sizeDword);
	long columnsRead = RegQueryValueEx(registry->key, TEXT("Columns"), NULL, NULL, (LPBYTE)&controlData->sharedMem->columns, &sizeDword);
	long timeRead = RegQueryValueEx(registry->key, TEXT("Time"), NULL, NULL, (LPBYTE)&controlData->sharedMem->time, &sizeDword);

	

	if (rowsRead != ERROR_SUCCESS || columnsRead != ERROR_SUCCESS || timeRead != ERROR_SUCCESS) {
		_tprintf_s(TEXT("\nCouldnt read the values for the pipe game!"));
		return FALSE;
	}
	return TRUE;
}

int initBoard(ControlData* data) {

	data->sharedMem->board = (TCHAR*)malloc((data->sharedMem->rows * data->sharedMem->columns) * sizeof(TCHAR));

	if (data->sharedMem->board != NULL) {
		_tprintf(TEXT("\nBoard loaded!\n"));
	}else {
		_tprintf(TEXT("\nMemory alocation wasnt done successfully\n"));
		return -1;
	}

	for (DWORD i = 0; i < data->sharedMem->rows * data->sharedMem->columns; i++) {
		_tcscpy_s(&data->sharedMem->board[i], sizeof(TCHAR), TEXT("━"));
	}

	return 1;
}

void startGame(Game* game) {
	int randomRow = -1;
	int randomColumn = -1;
	int number;
	srand(time(0));
	int quadrante = 0;
	game->finished = FALSE;

	game->pieces = (TCHAR*)malloc(6 * sizeof(TCHAR));
	_tcscpy_s(&game->pieces[0], sizeof(TCHAR), TEXT("━"));
	_tcscpy_s(&game->pieces[1], sizeof(TCHAR), TEXT("┃"));
	_tcscpy_s(&game->pieces[2], sizeof(TCHAR), TEXT("┏"));
	_tcscpy_s(&game->pieces[3], sizeof(TCHAR), TEXT("┓"));
	_tcscpy_s(&game->pieces[4], sizeof(TCHAR), TEXT("┛"));
	_tcscpy_s(&game->pieces[5], sizeof(TCHAR), TEXT("┗"));

	number = rand() % 2 + 1;

	if (number == 1) {
		randomRow = rand() % game->rows;

		number = rand() % 2 + 1;

		if (number == 1)
			randomColumn = 0;
		else
			randomColumn = game->columns - 1;
	}
	else {
		randomColumn = rand() % game->columns;

		number = rand() % 2 + 1;

		if (number == 1)
			randomRow = 0;
		else
			randomRow = game->rows - 1;
	}
	game->board[randomRow * game->rows + randomColumn] = 'B';

	if (randomRow < game->rows / 2 && randomColumn < game->columns / 2)
		quadrante = 1;
	else if (randomRow < game->rows / 2 && randomColumn >= game->columns / 2)
		quadrante = 2;
	else if (randomRow >= game->rows / 2 && randomColumn < game->columns / 2)
		quadrante = 3;
	else
		quadrante = 4;

	if (quadrante == 1) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRow = (rand() % (game->rows/2) + (game->rows/2));
			randomColumn = game->columns - 1;
		}
		else {
			randomColumn = (rand() % (game->columns/2) + (game->columns/2));
			randomRow = game->rows - 1;
		}
		game->board[randomRow * game->rows + randomColumn] = 'E';
	}else if (quadrante == 2) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRow = (rand() % (game->rows / 2) + (game->rows / 2));
			randomColumn = 0;
		}
		else {
			randomColumn = rand() % (game->columns / 2);
			randomRow = game->rows-1;
		}
		game->board[randomRow * game->rows + randomColumn] = 'E';
	}else if (quadrante == 3) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRow = rand() % (game->rows / 2);
			randomColumn = game->columns-1;
		}
		else {
			randomColumn = (rand() % (game->columns / 2) + (game->columns/2));
			randomRow = 0;
		}
		game->board[randomRow * game->rows + randomColumn] = 'E';
	}else if (quadrante == 4) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRow = rand() % (game->rows / 2);
			randomColumn = 0;
		}
		else {
			randomColumn = rand() % (game->columns / 2);
			randomRow = 0;
		}
		game->board[randomRow * game->rows + randomColumn] = 'E';
	}
}

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
// Default code for windows32 API
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

// Variables
	Game game;
	Registry registry;
	ControlData controlData;
	DWORD lMaximumSem = 1;
	controlData.sharedMem = &game;
	_tcscpy_s(registry.keyCompletePath, BUFFER, TEXT("SOFTWARE\\PipeGame\0"));
	controlData.hThreadTime = CreateThread(NULL, 0, &decreaseTime, &controlData.sharedMem, CREATE_SUSPENDED, NULL);
	TCHAR option[BUFFER];


	_tprintf(TEXT("\n-------------------PIPEGAME---------------\n\n"));


	if (argc != 4) {
		if (!configGame(&registry, &controlData)) {
			_tprintf(_T("\n\nError during configuration of the game!\n"));
			exit(1);
		}
	}else {
		game.rows = _ttoi(argv[1]);
		game.columns =  _ttoi(argv[2]);
		game.time =  _ttoi(argv[3]);
	}

	if (!initMemAndSync(&controlData)) {
		_tprintf(_T("Error creating/opening shared memory and synchronization mechanisms\n"));
		exit(1);
	}

	// Waits for the semaphores
	WaitForSingleObject(controlData.hWriteSem, INFINITE);
	WaitForSingleObject(controlData.hReadSem, INFINITE);

	// Function to start the game
	startGame(&game);
	ResumeThread(controlData.hThreadTime);
	
	// Shows the board for the game
	showBoard(&game);

	while (_ttoi(option) != 4) {
		_tprintf(TEXT("\n1 - List Players(in development)"));
		_tprintf(TEXT("\n2 - Suspend Game(in development)"));
		_tprintf(TEXT("\n3 - Resume Game(in development)"));
		_tprintf(TEXT("\n4 - Quit\n\n\n>"));

		_fgetts(option, BUFFER, stdin);

		switch (_ttoi(option)) {
		case 4:
			_tprintf(TEXT("\nClosing the application...\n"));
			game.finished = TRUE;
			break;
		default: 
			_tprintf(TEXT("\nCouldn´t recognize the command!\n"));
			break;
		}
	}

	// Waits for the thread to end
	//WaitForSingleObject(controlData.hThreadTime, INFINITE);

	// Frees the memory of the board
	free(controlData.sharedMem->board);
	free(controlData.sharedMem->pieces);

	// Release the semaphores
	ReleaseSemaphore(controlData.hWriteSem, lMaximumSem, NULL);
	ReleaseSemaphore(controlData.hReadSem, lMaximumSem, NULL);

	// Closing of all the handles
	RegCloseKey(registry.key);
	UnmapViewOfFile(controlData.sharedMem);
	CloseHandle(controlData.hMapFile);
	CloseHandle(controlData.hMutex);
	CloseHandle(controlData.hWriteSem);
	CloseHandle(controlData.hReadSem);
	return 0;
}