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

typedef struct _Game{
	TCHAR board[20][20];
	DWORD rows;
	DWORD columns;
	DWORD time;
	DWORD begginingR;
	DWORD begginingC;
	DWORD endR;
	DWORD endC;
	TCHAR pieces[6];
}Game;

typedef struct _Registry{
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
	unsigned int shutdown; // Release
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


DWORD WINAPI sendData(LPVOID p) {
	ControlData* data = (ControlData*)p;

	while (data->shutdown == 0) {
		WaitForSingleObject(data->hMutexPlay, INFINITE);
		WaitForSingleObject(data->hWriteSem, INFINITE);
		WaitForSingleObject(data->hMutex, INFINITE);
		CopyMemory(&(data->sharedMemGame->game), data->game, sizeof(Game));
		ReleaseMutex(data->hMutex);
		ReleaseMutex(data->hMutexPlay);
		ReleaseSemaphore(data->hReadSem, 1, NULL);
	}
	return 1;
}

DWORD WINAPI decreaseTime(LPVOID p) {
	ControlData* data = (ControlData*)p;

	while (data->shutdown == 0) {
		if(data->game->time > 0){
			(data->game->time)--;
			Sleep(1000);
		}
		else {
			data->shutdown = 1;
		}
	}
	return 1;
}

void play(ControlData* controlData) {
	unsigned int row = 50;
	unsigned int column = 50;
	unsigned int number = 0;
	TCHAR option[BUFFER];


	while (1) {
		WaitForSingleObject(controlData->hMutexPlay, INFINITE);
		number = rand() % 6;
		_tprintf(TEXT("\nPiece: %c\n"), controlData->game->pieces[number]);


		do {
			while (row >= controlData->game->rows) {
				_tprintf(TEXT("\nRow: "));
				_fgetts(option, BUFFER, stdin);
				row = _ttoi(option);

				if (row >= controlData->game->rows)
					_tprintf(TEXT("\nRows have to be between 0 and %d\n"), controlData->game->rows);
			}

			while (column >= controlData->game->columns) {
				_tprintf(TEXT("\nColumn: "));
				_fgetts(option, BUFFER, stdin);
				column = _ttoi(option);

				if (column >= controlData->game->columns)
					_tprintf(TEXT("\nColumn have to be between 0 and %d\n"), controlData->game->columns);
			}

			if ((row == controlData->game->begginingR && column == controlData->game->begginingC) || (row == controlData->game->endR && column == controlData->game->endC)) {
				_tprintf(TEXT("\nYou cant play neither in the beginning or end position.\n"));
				row = 50;
				column = 50;
			}
		} while ((row == controlData->game->begginingR && column == controlData->game->begginingC) || (row == controlData->game->endR && column == controlData->game->endC));
		controlData->game->board[row][column] = controlData->game->pieces[number];


		row = 50;
		column = 50;
		ReleaseMutex(controlData->hMutexPlay);
	}
}

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

DWORD WINAPI waterFlow(LPVOID p) {
	ControlData* data = (ControlData*)p;
	DWORD waterRow = data->game->begginingR;
	DWORD waterColumns = data->game->begginingC;
	TCHAR piece = TEXT('\0');
	DWORD win = 0;//while 0 game has no winner
	DWORD begin = 0;//while 0 water has not started flowing
	DWORD end = 0;//while 0 game has not finished

	piece = data->game->board[waterRow][waterColumns];

	while (end == 0) {
		if (data->game->time <= 100) {
			_tprintf(TEXT("CURRENT PIECE: %c"), piece);

			if (waterRow == data->game->endR && waterColumns == data->game->endC) {
				win = 1;
				end = 1;
				continue;
			}

			if (begin == 0) {
				//first row, no border
				if (waterRow == 0 && waterColumns != 0 && waterColumns != data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[3]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns + 1]);
							piece = data->game->board[waterRow][waterColumns + 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns + 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							continue;
						}

					}
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[2]) {
							piece = data->game->board[waterRow][waterColumns - 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns - 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[4] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[5]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow + 1][waterColumns]);
							piece = data->game->board[waterRow + 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow + 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							continue;
						}
					}

					//end = 1;
					continue;
				}

				//first row, left border
				if (waterRow == 0 && waterColumns == 0) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[3]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns + 1]);
							piece = data->game->board[waterRow][waterColumns + 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns + 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[5]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow + 1][waterColumns]);
							piece = data->game->board[waterRow + 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow + 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							continue;
						}
					}

					//end = 1;
					continue;
				}

				//first row, right border
				if (waterRow == 0 && waterColumns == data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[2]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns - 1]);
							piece = data->game->board[waterRow][waterColumns - 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns - 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[4]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow + 1][waterColumns]);
							piece = data->game->board[waterRow + 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow + 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							continue;
						}
					}

					//end = 1;
					continue;
				}

				//last row, no border
				if (waterRow == data->game->rows - 1 && waterColumns != 0 && waterColumns != data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[4]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns + 1]);
							piece = data->game->board[waterRow][waterColumns + 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns + 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[5]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns - 1]);
							piece = data->game->board[waterRow][waterColumns - 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns - 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[2] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[3]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow - 1][waterColumns]);
							piece = data->game->board[waterRow - 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow - 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							continue;
						}
					}

					//end = 1;
					continue;
				}

				//last row, left border
				if (waterRow == data->game->rows - 1 && waterColumns == 0) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[4]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns + 1]);
							piece = data->game->board[waterRow][waterColumns + 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns + 1], sizeof(TCHAR), TEXT('*'));
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[2]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow - 1][waterColumns]);
							piece = data->game->board[waterRow - 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow - 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							continue;
						}
					}

					//end = 1;
					continue;
				}

				//last row, right border
				if (waterRow == data->game->rows - 1 && waterColumns == data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns - 1] != '_') {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[5]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns - 1]);
							piece = data->game->board[waterRow][waterColumns - 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns - 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != '_') {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[3]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow - 1][waterColumns]);
							piece = data->game->board[waterRow - 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow - 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							continue;
						}
					}

					//end = 1;
					continue;
				}

				//first column, no border
				if (waterColumns == 0 && waterRow != 0 && waterRow != data->game->rows - 1) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[3] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[4]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns + 1]);
							piece = data->game->board[waterRow][waterColumns + 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns + 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[2] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[3]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow - 1][waterColumns]);
							piece = data->game->board[waterRow - 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow - 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;

							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[2]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow + 1][waterColumns]);
							piece = data->game->board[waterRow + 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow + 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							continue;
						}
					}

					//end = 1;
					continue;
				}

				//last column, no border
				if (waterColumns == data->game->columns - 1 && waterRow != 0 && waterRow != data->game->rows - 1) {
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[3] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[5]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns - 1]);
							piece = data->game->board[waterRow][waterColumns - 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns - 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[3]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow - 1][waterColumns]);
							piece = data->game->board[waterRow - 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow - 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[4]) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow + 1][waterColumns]);
							piece = data->game->board[waterRow + 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow + 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							continue;
						}
					}

					//end = 1;
					continue;
				}
			}
			//water is on a pipe
			if (piece == TEXT('━')) {
				_tprintf(TEXT("\n\n[DEBUG] I'm here!\n\n"));
				if (waterColumns != data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('B') && data->game->board[waterRow][waterColumns + 1] != TEXT('*')) {
						if (data->game->board[waterRow][waterColumns + 1] == TEXT('┓') || data->game->board[waterRow][waterColumns + 1] == TEXT('┛') || data->game->board[waterRow][waterColumns + 1] == TEXT('━') || data->game->board[waterRow][waterColumns + 1] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns + 1]);
							piece = data->game->board[waterRow][waterColumns + 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns + 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							continue;
						}
						end = 1;
					}
				}
				if (waterColumns != 0) {
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('B') && data->game->board[waterRow][waterColumns - 1] != TEXT('*')) {
						if (data->game->board[waterRow][waterColumns - 1] == TEXT('┏') || data->game->board[waterRow][waterColumns - 1] == TEXT('┗') || data->game->board[waterRow][waterColumns - 1] == TEXT('━') || data->game->board[waterRow][waterColumns - 1] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns - 1]);
							piece = data->game->board[waterRow][waterColumns - 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns - 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							continue;
						}
						end = 1;
					}

				}
				//end = 1;
				continue;
			}
			if (piece == TEXT('┃')) {
				_tprintf(TEXT("\n\n[DEBUG] I'm here!\n\n"));
				if (waterRow != data->game->rows - 1) {
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('B') && data->game->board[waterRow + 1][waterColumns] != TEXT('*')) {
						if (data->game->board[waterRow + 1][waterColumns] == TEXT('┛') || data->game->board[waterRow + 1][waterColumns] == TEXT('┗') || data->game->board[waterRow + 1][waterColumns] == TEXT('┃') || data->game->board[waterRow + 1][waterColumns] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow + 1][waterColumns]);
							piece = data->game->board[waterRow + 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow + 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							continue;
						}
						end = 1;
					}
				}
				if (waterRow != 0) {
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('B') && data->game->board[waterRow - 1][waterColumns] != TEXT('*')) {
						if (data->game->board[waterRow - 1][waterColumns] == TEXT('┏') || data->game->board[waterRow - 1][waterColumns] == TEXT('┓') || data->game->board[waterRow - 1][waterColumns] == TEXT('┃') || data->game->board[waterRow - 1][waterColumns] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow - 1][waterColumns]);
							piece = data->game->board[waterRow - 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow - 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							continue;
						}
						end = 1;
					}

				}

				//end = 1;
				continue;
			}
			if (piece == TEXT('┏')) {
				_tprintf(TEXT("\n\n[DEBUG] I'm here!\n\n"));
				if (data->game->board[waterRow][waterColumns + 1] != TEXT('*') && data->game->board[waterRow][waterColumns + 1] != TEXT('B')) {
					if (waterColumns != data->game->columns - 1) {
						if (data->game->board[waterRow][waterColumns + 1] == TEXT('┓') || data->game->board[waterRow][waterColumns + 1] == TEXT('┛') || data->game->board[waterRow][waterColumns + 1] == TEXT('━') || data->game->board[waterRow][waterColumns + 1] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns + 1]);
							piece = data->game->board[waterRow][waterColumns + 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns + 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							continue;
						}
						end = 1;
					}

					//end = 1;
					continue;
				}
				if (data->game->board[waterRow + 1][waterColumns] != TEXT('*') && data->game->board[waterRow + 1][waterColumns] != TEXT('B')) {
					if (waterRow != data->game->rows - 1) {
						if (data->game->board[waterRow + 1][waterColumns] == TEXT('┛') || data->game->board[waterRow + 1][waterColumns] == TEXT('┗') || data->game->board[waterRow + 1][waterColumns] == TEXT('┃') || data->game->board[waterRow + 1][waterColumns] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow + 1][waterColumns]);
							piece = data->game->board[waterRow + 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow + 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							continue;
						}
						end = 1;
					}

					//end = 1;
					continue;
				}

				//end = 1;
				continue;
			}
			if (piece == TEXT('┓')) {
				_tprintf(TEXT("\n\n[DEBUG] I'm here!\n\n"));
				if (data->game->board[waterRow][waterColumns - 1] != TEXT('*') && data->game->board[waterRow][waterColumns - 1] != TEXT('B')) {
					if (waterColumns != 0) {
						if (data->game->board[waterRow][waterColumns - 1] == TEXT('┏') || data->game->board[waterRow][waterColumns - 1] == TEXT('┗') || data->game->board[waterRow][waterColumns - 1] == TEXT('━') || data->game->board[waterRow][waterColumns - 1] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns - 1]);
							piece = data->game->board[waterRow][waterColumns - 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns - 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							continue;
						}
						end = 1;
					}

					//end = 1;
					continue;
				}
				if (data->game->board[waterRow + 1][waterColumns] != TEXT('*') && data->game->board[waterRow + 1][waterColumns] != TEXT('B')) {
					if (waterRow != data->game->rows - 1) {
						if (data->game->board[waterRow + 1][waterColumns] == TEXT('┛') || data->game->board[waterRow + 1][waterColumns] == TEXT('┗') || data->game->board[waterRow + 1][waterColumns] == TEXT('┃') || data->game->board[waterRow + 1][waterColumns] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow + 1][waterColumns]);
							piece = data->game->board[waterRow + 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow + 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							continue;
						}
						end = 1;
					}
					//end = 1;
					continue;
				}

				//end = 1;
				continue;
			}
			if (piece == TEXT('┛')) {
				_tprintf(TEXT("\n\n[DEBUG] I'm here!\n\n"));
				if (data->game->board[waterRow][waterColumns - 1] != TEXT('*') && data->game->board[waterRow][waterColumns - 1] != TEXT('B')) {
					if (waterColumns != 0) {
						if (data->game->board[waterRow][waterColumns - 1] == TEXT('┏') || data->game->board[waterRow][waterColumns - 1] == TEXT('┗') || data->game->board[waterRow][waterColumns - 1] == TEXT('━') || data->game->board[waterRow][waterColumns - 1] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns - 1]);
							piece = data->game->board[waterRow][waterColumns - 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns - 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							continue;
						}
						end = 1;
					}

					//end = 1;
					continue;
				}
				if (data->game->board[waterRow - 1][waterColumns] != TEXT('*') && data->game->board[waterRow - 1][waterColumns] != TEXT('B')) {
					if (waterRow != 0) {
						if (data->game->board[waterRow - 1][waterColumns] == TEXT('┓') || data->game->board[waterRow - 1][waterColumns] == TEXT('┏') || data->game->board[waterRow - 1][waterColumns] == TEXT('┃') || data->game->board[waterRow - 1][waterColumns] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow - 1][waterColumns]);
							piece = data->game->board[waterRow - 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow - 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							continue;
						}
						end = 1;
					}

					//end = 1;
					continue;
				}

				//end = 1;
				continue;
			}
			if (piece == TEXT('┗')) {
				_tprintf(TEXT("\n\n[DEBUG] I'm here!\n\n"));
				if (data->game->board[waterRow][waterColumns + 1] != TEXT('*') && data->game->board[waterRow][waterColumns + 1] != TEXT('B')) {
					if (waterColumns != data->game->columns - 1) {
						if (data->game->board[waterRow][waterColumns + 1] == TEXT('┓') || data->game->board[waterRow][waterColumns + 1] == TEXT('┛') || data->game->board[waterRow][waterColumns + 1] == TEXT('━') || data->game->board[waterRow][waterColumns + 1] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow][waterColumns + 1]);
							piece = data->game->board[waterRow][waterColumns + 1];
							//_tcscpy_s(&data->game->board[waterRow][waterColumns + 1], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							continue;
						}
						end = 1;
					}


					continue;
				}
				if (data->game->board[waterRow - 1][waterColumns] != TEXT('*') && data->game->board[waterRow - 1][waterColumns] != TEXT('B')) {
					if (waterRow != 0) {
						if (data->game->board[waterRow - 1][waterColumns] == TEXT('┓') || data->game->board[waterRow - 1][waterColumns] == TEXT('┏') || data->game->board[waterRow - 1][waterColumns] == TEXT('┃') || data->game->board[waterRow - 1][waterColumns] == TEXT('E')) {
							//_tcscpy_s(&piece, sizeof(TCHAR*), &data->game->board[waterRow - 1][waterColumns]);
							piece = data->game->board[waterRow - 1][waterColumns];
							//_tcscpy_s(&data->game->board[waterRow - 1][waterColumns], sizeof(TCHAR), TEXT("*"));
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							continue;
						}
						end = 1;
					}
					continue;
				}


			}

		}
	}
	if (end == 1 && win == 0) {
		_tprintf(TEXT("\n\nYOU LOST.\n\n"));
		showBoard(data);
		exit(1);
	}
	if (win == 1) {
		_tprintf(TEXT("\n\nYOU WON.\n\n"));
		data->game->board[data->game->endR][data->game->endC] = TEXT('E');
		showBoard(data);
		exit(1);
	}
}


DWORD WINAPI receiveCommnadsMonitor(LPVOID p) {
	ControlData* data = (ControlData*)p;
	DWORD command;
	int i = 0;


	do {
		WaitForSingleObject(data->commandEvent, INFINITE);
		WaitForSingleObject(data->commandMutex, INFINITE);
		if (i == BUFFERSIZE)
			i = 0;
		CopyMemory(&command, &(data->sharedMemCommand->commandMonitor[i]), sizeof(DWORD));
		i++;
		ReleaseMutex(data->commandMutex);

		if (command == 1) {
			_tprintf(TEXT("\nTIME LEFT: [%d]\n"), data->game->time);
		}
		if (command == 2) {
			_tprintf(TEXT("insert"));
		}
		if (command == 3) {
			_tprintf(TEXT("random"));
		}
	} while (data->shutdown != 1);


	return 1;

}

BOOL initMemAndSync(ControlData* p) {
	BOOL firstProcess = FALSE;
	_tprintf(TEXT("\n\nConfigs for the game initializing...\n"));

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
		_tprintf(TEXT("\nError: MapViewOfFile (%d)."), GetLastError());
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

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("\nYou cant run two servers at once, shutting down.\n"));
		UnmapViewOfFile(p->sharedMemGame);
		CloseHandle(p->hMapFileGame);
		CloseHandle(p->hMutex);
		CloseHandle(p->hWriteSem);
		CloseHandle(p->hReadSem);
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
		CloseHandle(p->commandMutex);
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

	for (DWORD i = 0; i < data->game->rows; i++) {
		for (DWORD j = 0; j < data->game->columns; j++) {
			_tcscpy_s(&data->game->board[i][j], sizeof(TCHAR), TEXT("-"));
		}
	}

	return 1;
}

void startGame(ControlData* data) {
	DWORD randomRowB = -1;
	DWORD randomColumnB = -1;
	DWORD randomRowE = -1;
	DWORD randomColumnE = -1;
	int number;
	int quadrante = 0;

	_tcscpy_s(&data->game->pieces[0], sizeof(TCHAR), TEXT("━"));
	_tcscpy_s(&data->game->pieces[1], sizeof(TCHAR), TEXT("┃"));
	_tcscpy_s(&data->game->pieces[2], sizeof(TCHAR), TEXT("┏"));
	_tcscpy_s(&data->game->pieces[3], sizeof(TCHAR), TEXT("┓"));
	_tcscpy_s(&data->game->pieces[4], sizeof(TCHAR), TEXT("┛"));
	_tcscpy_s(&data->game->pieces[5], sizeof(TCHAR), TEXT("┗"));

	number = rand() % 2 + 1;

	if (number == 1) {
		randomRowB = rand() % data->game->rows;

		number = rand() % 2 + 1;

		if (number == 1)
			randomColumnB = 0;
		else
			randomColumnB = data->game->columns - 1;
	}
	else {
		randomColumnB = rand() % data->game->columns;

		number = rand() % 2 + 1;

		if (number == 1)
			randomRowB = 0;
		else
			randomRowB = data->game->rows - 1;
	}

	data->game->begginingR = randomRowB;
	data->game->begginingC = randomColumnB;

	if (randomRowB < data->game->rows / 2 && randomColumnB < data->game->columns / 2)
		quadrante = 1;
	else if (randomRowB < data->game->rows / 2 && randomColumnB >= data->game->columns / 2)
		quadrante = 2;
	else if (randomRowB >= data->game->rows / 2 && randomColumnB < data->game->columns / 2)
		quadrante = 3;
	else
		quadrante = 4;

	if (quadrante == 1) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRowE = (rand() % (data->game->rows /2) + (data->game->rows /2));
			randomColumnE = data->game->columns - 1;
		}
		else {
			randomColumnE = (rand() % (data->game->columns/2) + (data->game->columns/2));
			randomRowE = data->game->rows - 1;
		}
	}else if (quadrante == 2) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRowE = (rand() % (data->game->rows / 2) + (data->game->rows / 2));
			randomColumnE = 0;
		}
		else {
			randomColumnE = rand() % (data->game->columns / 2);
			randomRowE = data->game->rows-1;
		}
	}else if (quadrante == 3) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRowE = rand() % (data->game->rows / 2);
			randomColumnE = data->game->columns-1;
		}
		else {
			randomColumnE = (rand() % (data->game->columns / 2) + (data->game->columns/2));
			randomRowE = 0;
		}
	}else if(quadrante == 4) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRowE = rand() % (data->game->rows/2);
			randomColumnE = 0;
		}
		else {
			randomColumnE = rand() % (data->game->columns / 2);
			randomRowE = 0;
		}
		
	}

	data->game->board[randomRowB][randomColumnB] = 'B';
	data->game->board[randomRowE][randomColumnE] = 'E';
	
	data->game->endR = randomRowE;
	data->game->endC = randomColumnE;
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
	TCHAR option[BUFFER];
	HANDLE hThreadSendDataToMonitor;
	HANDLE receiveCommandsMonitorThread;
	HANDLE hThreadTime;
	HANDLE waterFlowThread;
	controlData.shutdown = 0; // trinco 
	controlData.count = 0; // numero de itens
	boolean firstTime = TRUE;
	srand((unsigned int)time(NULL));



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

		_tprintf(TEXT("\nBoard Loaded with [%d] rows and [%d] columns, water [%d] seconds.\n"), controlData.game->rows, controlData.game->columns, controlData.game->time);
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

	hThreadTime = CreateThread(NULL, 0, decreaseTime, &controlData, CREATE_SUSPENDED, NULL);
	if (hThreadTime == NULL) {
		_tprintf(TEXT("\nCouldnt create thread to decrease the time of the game.\n"));
		exit(1);
	}
	waterFlowThread = CreateThread(NULL, 0, waterFlow, &controlData, 0, NULL);
	if (waterFlowThread == NULL) {
		_tprintf(TEXT("\nCouldn't create thread to control the water flow.\n"));
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

		case 5:
			if(firstTime){
				ResumeThread(hThreadTime);
				play(&controlData);
			}
			else
				play(&controlData);
		default: 
			_tprintf(TEXT("\nCouldn´t recognize the command.\n"));
			break;
		}
	}
	
	// Waiting for the thread to end
	WaitForSingleObject(receiveCommandsMonitorThread, INFINITE);
	WaitForSingleObject(hThreadTime, INFINITE);
	WaitForSingleObject(hThreadSendDataToMonitor, INFINITE);


	// Closing of all the handles
	RegCloseKey(registry.key);
	UnmapViewOfFile(controlData.sharedMemGame);
	UnmapViewOfFile(controlData.sharedMemCommand);
	CloseHandle(hThreadTime);
	CloseHandle(hThreadSendDataToMonitor);
	CloseHandle(receiveCommandsMonitorThread);
	CloseHandle(controlData.commandMutex);
	CloseHandle(controlData.commandEvent);
	CloseHandle(controlData.hMapFileGame);
	CloseHandle(controlData.hMapFileMemory);
	CloseHandle(controlData.hMutexPlay);
	CloseHandle(controlData.hMutex);
	CloseHandle(controlData.hWriteSem);
	CloseHandle(controlData.hReadSem);
	return 0;
}