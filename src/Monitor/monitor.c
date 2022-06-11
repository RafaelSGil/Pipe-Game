#include <io.h>
#include <Windows.h>
#include <fcntl.h>
#include <stdio.h>	
#include <stdlib.h>
#include <tchar.h>
#include <stdbool.h>

#include "../Server/Names.h"
#include "../Server/Game.h"
#include "../Server/ControlData.h"


void showBoard(ControlData* data) {
	WaitForSingleObject(data->hMutex, INFINITE);
		if(data->game[0].gameType == 1){
			_tprintf(TEXT("\n\nTime: [%d]\n\n"), data->game[0].time);
			for (DWORD i = 0; i < data->game[0].rows; i++)
			{
				_tprintf(TEXT("\n"));
				for (DWORD j = 0; j < data->game[0].columns; j++)
					_tprintf(TEXT("%c "), data->game[0].board[i][j]);
			}
				_tprintf(TEXT("\n\n"));
		}
		else {
			for (int k = 0; k < 2; k++) {
					_tprintf(TEXT("\n\nTime: [%d]\n\n"), data->game[k].time);
					for (DWORD i = 0; i < data->game[k].rows; i++)
					{
						_tprintf(TEXT("\n"));
						for (DWORD j = 0; j < data->game[k].columns; j++)
							_tprintf(TEXT("%c "), data->game[k].board[i][j]);
					}
					_tprintf(TEXT("\n\n"));
			}
		}
	ReleaseMutex(data->hMutex);
}


DWORD WINAPI receiveData(LPVOID p) {
	ControlData* data = (ControlData*)p;

	while (1) {
		if (data->game[0].shutdown == 1)
			return 0;
		WaitForSingleObject(data->hReadSem, 2000);
		WaitForSingleObject(data->hMutex, 2000);
		CopyMemory(&(data->game[0]), &(data->sharedMemGame->game[0]), sizeof(Game));
		if (data->game[0].gameType == 2) {
			CopyMemory(&(data->game[1]), &(data->sharedMemGame->game[1]), sizeof(Game));
		}
		ReleaseMutex(data->hMutex);
		ReleaseSemaphore(data->hWriteSem, 1, NULL);

	}
}

DWORD WINAPI showBoardAlways(LPVOID p) {
	ControlData* data = (ControlData*)p;
	while (data->game->shutdown == 0) {
		if (data->game->shutdown == 1)
			break;
		Sleep(1000);
		system("cls");
		showBoard(data);
		_tprintf(TEXT("\n>"));
	}
	return (1);
}

DWORD WINAPI executeCommands(LPVOID p) {
	ControlData* data = (ControlData*)p;
	TCHAR* token = NULL;
	TCHAR* nextToken = NULL;
	TCHAR option[BUFFER] = TEXT(" ");

	Command command;
	command.command = 0;
	command.parameter = 0;
	command.paramter1 = 0;
	int i = 0;

	do {

		_getts_s(option, _countof(option));
		token = _tcstok_s(option, TEXT(" "), &nextToken);

		if (_tcscmp(token, TEXT("delay")) == 0) {
			WaitForSingleObject(data->commandMutex, INFINITE);
			if (i == BUFFERSIZE)
				i = 0;
			command.command = 1;
			token = _tcstok_s(NULL, TEXT(" "), &nextToken);
			if (token != NULL)
				command.parameter = _ttoi(token);
			if (command.parameter != 0 && token != NULL)
				CopyMemory(&(data->sharedMemCommand->commandMonitor[i]), &command, sizeof(Command));
			i++;
			ReleaseMutex(data->commandMutex);
			SetEvent(data->commandEvent);
			ResetEvent(data->commandEvent);
		}
		else if (_tcscmp(token, TEXT("insert")) == 0) {
			WaitForSingleObject(data->commandMutex, INFINITE);
			if (i == BUFFERSIZE)
				i = 0;
			command.command = 2;
			token = _tcstok_s(NULL, TEXT(" "), &nextToken);
			if (token != NULL)
				command.parameter = _ttoi(token);
			token = _tcstok_s(NULL, TEXT(" "), &nextToken);
			if (token != NULL)
				command.paramter1 = _ttoi(token);
			if (token != NULL)
				CopyMemory(&(data->sharedMemCommand->commandMonitor[i]), &command, sizeof(Command));
			i++;
			ReleaseMutex(data->commandMutex);
			SetEvent(data->commandEvent);
			ResetEvent(data->commandEvent);
		}
		else if (_tcscmp(token, TEXT("random")) == 0) {
			WaitForSingleObject(data->commandMutex, INFINITE);
			if (i == BUFFERSIZE)
				i = 0;
			command.command = 3;
			CopyMemory(&(data->sharedMemCommand->commandMonitor[i]), &command, sizeof(Command));
			i++;
			ReleaseMutex(data->commandMutex);
			SetEvent(data->commandEvent);
			ResetEvent(data->commandEvent);
		}
		else if (_tcscmp(token, TEXT("help")) == 0) {
			_tprintf(TEXT("\nCommands\n"));
			_tprintf(TEXT("\n1 - show"));
			_tprintf(TEXT("\n2 - delay 'time'"));
			_tprintf(TEXT("\n3 - insert 'row' 'column'"));
			_tprintf(TEXT("\n4 - random"));
			_tprintf(TEXT("\n5 - exit\n\nCommand: "));
		}
		else {
			_tprintf(TEXT("\nCouldnt recognize command.\n"));
		}
	} while (_tcscmp(option, TEXT("exit")) != 0);

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
	return TRUE;
}





int _tmain(int argc, TCHAR** argv) {
#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif
	Game game[2];
	ControlData controlData;
	controlData.game[0] = game[0];
	controlData.game[1] = game[1];
	HANDLE hThreadReceiveDataFromServer;
	HANDLE executeCommandsThread;
	HANDLE showBoardThread;
	controlData.game->shutdown = 0;

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

	showBoardThread = CreateThread(NULL, 0, showBoardAlways, &controlData, 0, NULL);
	if (executeCommandsThread == NULL) {
		_tprintf(TEXT("\nCouldnt create thread to show board.\n"));
		exit(1);
	}

	while (1) {
		if (controlData.game->shutdown == 1) {
			_tprintf(TEXT("\nShutting down...\n"));
			break;
		}
	}
	// Waiting for the threads to end
	WaitForMultipleObjects(3, hThreadReceiveDataFromServer, executeCommandsThread, showBoardThread, TRUE, 2000);

	// Closing of all the handles
	UnmapViewOfFile(controlData.sharedMemCommand);
	UnmapViewOfFile(controlData.sharedMemGame);
	CloseHandle(controlData.hMapFileGame);
	CloseHandle(controlData.hMapFileMemory);
	CloseHandle(controlData.hMutex);
	CloseHandle(controlData.hWriteSem);
	CloseHandle(controlData.hReadSem);
	Sleep(3000);
	return 0;
}