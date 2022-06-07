#pragma once
#ifndef NAMES
#define SHM_NAME_GAME TEXT("fmGameSpace") // Name of the shared memory for the game
#define SHM_NAME_MESSAGE TEXT("fmMsgSpace") // Name of the shared memory for the message
#define MUTEX_NAME TEXT("fmMutex") // Name of the mutex   
#define MUTEX_NAME_PLAY TEXT("fmMutexPlay") // Name of the mutex   
#define SEM_WRITE_NAME TEXT("SEM_WRITE") // Name of the writting lightning 
#define SEM_READ_NAME TEXT("SEM_READ")	// Name of the reading lightning
#define EVENT_NAME TEXT("COMMANDEVENT") //Name of the command event
#define COMMAND_MUTEX_NAME TEXT("COMMANDMUTEX") //Name of the command mutex
#define BUFFER 256
#define SIZE_DWORD 257*(sizeof(DWORD))
#define BUFFERSIZE 10
#endif // !NAMES

