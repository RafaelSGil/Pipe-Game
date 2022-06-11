#pragma once
#ifndef CONTROLDATA
#include <Windows.h>
#include "Game.h"
#include "Pipes.h"

typedef struct _Command {
	DWORD command;
	DWORD parameter;
	DWORD paramter1;
}Command;


typedef struct _SharedMemGame {
	Game game[2];
}SharedMemGame;

typedef struct _SharedMemCommand {
	Command commandMonitor[256];
}SharedMemCommand;


typedef struct _ControlData { 
	HANDLE hMapFileGame; // Memory from the game
	HANDLE hMapFileMemory; // Memory from the message
	SharedMemGame* sharedMemGame; // Shared memory of the game
	SharedMemCommand* sharedMemCommand; // Shared memory of the command from the monitor
	HANDLE hMutex; // Mutex
	HANDLE hWriteSem; // Semaphore warns writting
	HANDLE hReadSem; // Semaphore warns reading 
	HANDLE commandEvent; //event used to coordinate commands received
	HANDLE commandMutex; //mutex used to coordinate commands
	Game game[2];
	threadData* data;
	DWORD time;
}ControlData;
#endif // !1
