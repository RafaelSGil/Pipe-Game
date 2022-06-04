#pragma once
#ifndef PIPES
#include <Windows.h>
#include "Game.h"
#define N 2
#define PIPE_NAME TEXT("\\\\.\\pipe\\PipeGame")

typedef struct {
	HANDLE hInstancia;
	OVERLAPPED overlap;
	BOOL activo;
}pipeData;

typedef struct {
	pipeData hPipes[N];
	HANDLE hEvents[N];
	HANDLE hMutex;
	int terminar;
}threadData;
#endif 