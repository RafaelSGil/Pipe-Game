#include <io.h>
#include <Windows.h>
#include <fcntl.h>
#include <stdio.h>	
#include <stdlib.h>
#include <tchar.h>
#include <stdbool.h>

#include "Names.h"
#include "Registry.h"
#include  "Game.h"
#include "ControlData.h"

DWORD WINAPI ThreadMensagens(LPVOID* param) {
	DWORD n;
	int i;
	BOOL ret;
	int number;
	ControlData* dados = (ControlData*)param;

	do {
		for (i = 0; i < N; i++) {
			WaitForSingleObject(dados->data->hMutex, INFINITE);
			if (dados->data->hPipes[i].activo) {
				if (!WriteFile(dados->data->hPipes[i].hInstancia, dados->game, sizeof(Game), &n, NULL))
					_tprintf(_T("[ERRO] Escrever no pipe! (WriteFile)\n"));
				else {
					_tprintf(_T("[ESCRITOR] Enviei %d bytes ao leitor [%d]... (WriteFile)\n"), n, i);
					ret = ReadFile(dados->data->hPipes[i].hInstancia, dados->game, sizeof(Game), &n, NULL);
					_tprintf(_T("[ESCRITOR] Recebi %d bytes\n"), n);
					WaitForSingleObject(dados->hMutex, INFINITE);
					dados->game->board[dados->game->row][dados->game->column] = dados->game->piece;
					ReleaseMutex(dados->hMutex);
				}
			}
			ReleaseMutex(dados->data->hMutex);
		}
	} while (dados->game->shutdown == 0);
	dados->data->terminar = 1;
	for (i = 0; i < N; i++)
		SetEvent(dados->data->hEvents[i]);
	return 0;
}

DWORD WINAPI sendData(LPVOID p) {
	ControlData* data = (ControlData*)p;

	while (data->game->shutdown == 0) {
		WaitForSingleObject(data->hWriteSem, INFINITE);
		WaitForSingleObject(data->hMutex, INFINITE);
		CopyMemory(&(data->sharedMemGame->game), data->game, sizeof(Game));
		ReleaseMutex(data->hMutex);
		ReleaseSemaphore(data->hReadSem, 1, NULL);
	}
	return 1;
}

DWORD WINAPI decreaseTime(LPVOID p) {
	ControlData* data = (ControlData*)p;

	while (data->game->shutdown == 0) {
		if (data->game->time > 0) {
			(data->game->time)--;
			Sleep(1000);
		}
		else
			return 1;

	}
	return 0;
}

void play(ControlData* controlData) {
	unsigned int row = 50;
	unsigned int column = 50;
	unsigned int number = 0;
	TCHAR option[BUFFER];


	while (1) {
		if (controlData->game->suspended == 1) {
			break;
		}
		if (controlData->game->random)
			number = rand() % 6;
		else {
			if (controlData->game->index == 6)
				controlData->game->index = 0;
			number = controlData->game->index;
		}

		_tprintf(TEXT("\n[-1 to suspend game]\n\nPiece: %c\n"), controlData->game->pieces[number]);


		do {
			while (row >= controlData->game->rows) {
				_tprintf(TEXT("\nRow: "));
				_fgetts(option, BUFFER, stdin);
				row = _ttoi(option);

				if (row == -1) {
					controlData->game->suspended = 1;
					break;
				}

				if (row >= controlData->game->rows)
					_tprintf(TEXT("\nRows have to be between 0 and %d\n"), controlData->game->rows);
			}

			if (controlData->game->suspended == 1) {
				break;
			}

			while (column >= controlData->game->columns) {
				_tprintf(TEXT("\nColumn: "));
				_fgetts(option, BUFFER, stdin);
				column = _ttoi(option);

				if (column == -1) {
					controlData->game->suspended = 1;
					break;
				}

				if (column >= controlData->game->columns)
					_tprintf(TEXT("\nColumn have to be between 0 and %d\n"), controlData->game->columns);
			}

			if (controlData->game->suspended == 1) {
				break;
			}

			if ((row == controlData->game->begginingR && column == controlData->game->begginingC) || (row == controlData->game->endR && column == controlData->game->endC)) {
				_tprintf(TEXT("\nYou cant play neither in the beginning or end position.\n"));
				row = 50;
				column = 50;
			}
		} while ((row == controlData->game->begginingR && column == controlData->game->begginingC) || (row == controlData->game->endR && column == controlData->game->endC));

		if (row != -1 && column != -1) {
			WaitForSingleObject(controlData->hMutex, INFINITE);
			controlData->game->board[row][column] = controlData->game->pieces[number];
			ReleaseMutex(controlData->hMutex);
		}

		controlData->game->index++;
		row = 50;
		column = 50;

	}
}

void showBoard(ControlData* data) {
	WaitForSingleObject(data->hMutex, INFINITE);
	_tprintf(TEXT("\n\nTime: [%d]\n\n"), data->game->time);
	for (DWORD i = 0; i < data->game->rows; i++)
	{
		_tprintf(TEXT("\n"));
		for (DWORD j = 0; j < data->game->columns; j++)
			_tprintf(TEXT("%c "), data->game->board[i][j]);
	}
	_tprintf(TEXT("\n\n"));
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
		if (data->game->time == 0) {

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
							piece = data->game->board[waterRow][waterColumns + 1];
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							Sleep(2000);
							continue;
						}

					}
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[2]) {
							piece = data->game->board[waterRow][waterColumns - 1];
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[4] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[5]) {
							piece = data->game->board[waterRow + 1][waterColumns];
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}

					end = 1;
					continue;
				}

				//first row, left border
				if (waterRow == 0 && waterColumns == 0) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[3]) {
							piece = data->game->board[waterRow][waterColumns + 1];
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[5]) {
							piece = data->game->board[waterRow + 1][waterColumns];
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}

					end = 1;
					continue;
				}

				//first row, right border
				if (waterRow == 0 && waterColumns == data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[2]) {
							piece = data->game->board[waterRow][waterColumns - 1];
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[4]) {
							piece = data->game->board[waterRow + 1][waterColumns];
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}

					end = 1;
					continue;
				}

				//last row, no border
				if (waterRow == data->game->rows - 1 && waterColumns != 0 && waterColumns != data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[4]) {
							piece = data->game->board[waterRow][waterColumns + 1];
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[5]) {
							piece = data->game->board[waterRow][waterColumns - 1];
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[2] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[3]) {
							piece = data->game->board[waterRow - 1][waterColumns];
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}

					end = 1;
					continue;
				}

				//last row, left border
				if (waterRow == data->game->rows - 1 && waterColumns == 0) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[4]) {
							piece = data->game->board[waterRow][waterColumns + 1];
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[2]) {
							piece = data->game->board[waterRow - 1][waterColumns];
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}

					end = 1;
					continue;
				}

				//last row, right border
				if (waterRow == data->game->rows - 1 && waterColumns == data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns - 1] != '_') {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[5]) {
							piece = data->game->board[waterRow][waterColumns - 1];
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != '_') {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[3]) {
							piece = data->game->board[waterRow - 1][waterColumns];
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}

					end = 1;
					continue;
				}

				//first column, no border
				if (waterColumns == 0 && waterRow != 0 && waterRow != data->game->rows - 1) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns + 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[3] || data->game->board[waterRow][waterColumns + 1] == data->game->pieces[4]) {
							piece = data->game->board[waterRow][waterColumns + 1];
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[2] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[3]) {
							piece = data->game->board[waterRow - 1][waterColumns];
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[2]) {
							piece = data->game->board[waterRow + 1][waterColumns];
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}

					end = 1;
					continue;
				}

				//last column, no border
				if (waterColumns == data->game->columns - 1 && waterRow != 0 && waterRow != data->game->rows - 1) {
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('_')) {
						if (data->game->board[waterRow][waterColumns - 1] == data->game->pieces[0] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[3] || data->game->board[waterRow][waterColumns - 1] == data->game->pieces[5]) {
							piece = data->game->board[waterRow][waterColumns - 1];
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow - 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow - 1][waterColumns] == data->game->pieces[3]) {
							piece = data->game->board[waterRow - 1][waterColumns];
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('_')) {
						if (data->game->board[waterRow + 1][waterColumns] == data->game->pieces[1] || data->game->board[waterRow + 1][waterColumns] == data->game->pieces[4]) {
							piece = data->game->board[waterRow + 1][waterColumns];
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							begin = 1;
							Sleep(2000);
							continue;
						}
					}

					end = 1;
					continue;
				}
			}
			//water is on a pipe
			if (piece == TEXT('━')) {
				if (waterColumns != data->game->columns - 1) {
					if (data->game->board[waterRow][waterColumns + 1] != TEXT('B') && data->game->board[waterRow][waterColumns + 1] != TEXT('*')) {
						if (data->game->board[waterRow][waterColumns + 1] == TEXT('┓') || data->game->board[waterRow][waterColumns + 1] == TEXT('┛') || data->game->board[waterRow][waterColumns + 1] == TEXT('━') || data->game->board[waterRow][waterColumns + 1] == TEXT('E')) {
							piece = data->game->board[waterRow][waterColumns + 1];
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							Sleep(2000);
							continue;
						}
						end = 1;
					}
				}
				if (waterColumns != 0) {
					if (data->game->board[waterRow][waterColumns - 1] != TEXT('B') && data->game->board[waterRow][waterColumns - 1] != TEXT('*')) {
						if (data->game->board[waterRow][waterColumns - 1] == TEXT('┏') || data->game->board[waterRow][waterColumns - 1] == TEXT('┗') || data->game->board[waterRow][waterColumns - 1] == TEXT('━') || data->game->board[waterRow][waterColumns - 1] == TEXT('E')) {
							piece = data->game->board[waterRow][waterColumns - 1];
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							Sleep(2000);
							continue;
						}
						end = 1;
					}

				}
				continue;
			}
			if (piece == TEXT('┃')) {
				if (waterRow != data->game->rows - 1) {
					if (data->game->board[waterRow + 1][waterColumns] != TEXT('B') && data->game->board[waterRow + 1][waterColumns] != TEXT('*')) {
						if (data->game->board[waterRow + 1][waterColumns] == TEXT('┛') || data->game->board[waterRow + 1][waterColumns] == TEXT('┗') || data->game->board[waterRow + 1][waterColumns] == TEXT('┃') || data->game->board[waterRow + 1][waterColumns] == TEXT('E')) {
							piece = data->game->board[waterRow + 1][waterColumns];
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							Sleep(2000);
							continue;
						}
						end = 1;
					}
				}
				if (waterRow != 0) {
					if (data->game->board[waterRow - 1][waterColumns] != TEXT('B') && data->game->board[waterRow - 1][waterColumns] != TEXT('*')) {
						if (data->game->board[waterRow - 1][waterColumns] == TEXT('┏') || data->game->board[waterRow - 1][waterColumns] == TEXT('┓') || data->game->board[waterRow - 1][waterColumns] == TEXT('┃') || data->game->board[waterRow - 1][waterColumns] == TEXT('E')) {
							piece = data->game->board[waterRow - 1][waterColumns];
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							Sleep(2000);
							continue;
						}
						end = 1;
					}

				}
				continue;
			}
			if (piece == TEXT('┏')) {
				if (data->game->board[waterRow][waterColumns + 1] != TEXT('*') && data->game->board[waterRow][waterColumns + 1] != TEXT('B')) {
					if (waterColumns != data->game->columns - 1) {
						if (data->game->board[waterRow][waterColumns + 1] == TEXT('┓') || data->game->board[waterRow][waterColumns + 1] == TEXT('┛') || data->game->board[waterRow][waterColumns + 1] == TEXT('━') || data->game->board[waterRow][waterColumns + 1] == TEXT('E')) {
							piece = data->game->board[waterRow][waterColumns + 1];
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							Sleep(2000);
							continue;
						}
						end = 1;
					}

					continue;
				}
				if (data->game->board[waterRow + 1][waterColumns] != TEXT('*') && data->game->board[waterRow + 1][waterColumns] != TEXT('B')) {
					if (waterRow != data->game->rows - 1) {
						if (data->game->board[waterRow + 1][waterColumns] == TEXT('┛') || data->game->board[waterRow + 1][waterColumns] == TEXT('┗') || data->game->board[waterRow + 1][waterColumns] == TEXT('┃') || data->game->board[waterRow + 1][waterColumns] == TEXT('E')) {
							piece = data->game->board[waterRow + 1][waterColumns];
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							Sleep(2000);
							continue;
						}
						end = 1;
					}

					continue;
				}

				continue;
			}
			if (piece == TEXT('┓')) {
				if (data->game->board[waterRow][waterColumns - 1] != TEXT('*') && data->game->board[waterRow][waterColumns - 1] != TEXT('B')) {
					if (waterColumns != 0) {
						if (data->game->board[waterRow][waterColumns - 1] == TEXT('┏') || data->game->board[waterRow][waterColumns - 1] == TEXT('┗') || data->game->board[waterRow][waterColumns - 1] == TEXT('━') || data->game->board[waterRow][waterColumns - 1] == TEXT('E')) {
							piece = data->game->board[waterRow][waterColumns - 1];
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							Sleep(2000);
							continue;
						}
						end = 1;
					}

					continue;
				}
				if (data->game->board[waterRow + 1][waterColumns] != TEXT('*') && data->game->board[waterRow + 1][waterColumns] != TEXT('B')) {
					if (waterRow != data->game->rows - 1) {
						if (data->game->board[waterRow + 1][waterColumns] == TEXT('┛') || data->game->board[waterRow + 1][waterColumns] == TEXT('┗') || data->game->board[waterRow + 1][waterColumns] == TEXT('┃') || data->game->board[waterRow + 1][waterColumns] == TEXT('E')) {
							piece = data->game->board[waterRow + 1][waterColumns];
							data->game->board[waterRow + 1][waterColumns] = TEXT('*');
							waterRow++;
							Sleep(2000);
							continue;
						}
						end = 1;
					}
					continue;
				}

				continue;
			}
			if (piece == TEXT('┛')) {
				if (data->game->board[waterRow][waterColumns - 1] != TEXT('*') && data->game->board[waterRow][waterColumns - 1] != TEXT('B')) {
					if (waterColumns != 0) {
						if (data->game->board[waterRow][waterColumns - 1] == TEXT('┏') || data->game->board[waterRow][waterColumns - 1] == TEXT('┗') || data->game->board[waterRow][waterColumns - 1] == TEXT('━') || data->game->board[waterRow][waterColumns - 1] == TEXT('E')) {
							piece = data->game->board[waterRow][waterColumns - 1];
							data->game->board[waterRow][waterColumns - 1] = TEXT('*');
							waterColumns--;
							Sleep(2000);
							continue;
						}
						end = 1;
					}

					continue;
				}
				if (data->game->board[waterRow - 1][waterColumns] != TEXT('*') && data->game->board[waterRow - 1][waterColumns] != TEXT('B')) {
					if (waterRow != 0) {
						if (data->game->board[waterRow - 1][waterColumns] == TEXT('┓') || data->game->board[waterRow - 1][waterColumns] == TEXT('┏') || data->game->board[waterRow - 1][waterColumns] == TEXT('┃') || data->game->board[waterRow - 1][waterColumns] == TEXT('E')) {
							piece = data->game->board[waterRow - 1][waterColumns];
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							Sleep(2000);
							continue;
						}
						end = 1;
					}

					continue;
				}

				continue;
			}
			if (piece == TEXT('┗')) {
				if (data->game->board[waterRow][waterColumns + 1] != TEXT('*') && data->game->board[waterRow][waterColumns + 1] != TEXT('B')) {
					if (waterColumns != data->game->columns - 1) {
						if (data->game->board[waterRow][waterColumns + 1] == TEXT('┓') || data->game->board[waterRow][waterColumns + 1] == TEXT('┛') || data->game->board[waterRow][waterColumns + 1] == TEXT('━') || data->game->board[waterRow][waterColumns + 1] == TEXT('E')) {
							piece = data->game->board[waterRow][waterColumns + 1];
							data->game->board[waterRow][waterColumns + 1] = TEXT('*');
							waterColumns++;
							Sleep(2000);
							continue;
						}
						end = 1;
					}


					continue;
				}
				if (data->game->board[waterRow - 1][waterColumns] != TEXT('*') && data->game->board[waterRow - 1][waterColumns] != TEXT('B')) {
					if (waterRow != 0) {
						if (data->game->board[waterRow - 1][waterColumns] == TEXT('┓') || data->game->board[waterRow - 1][waterColumns] == TEXT('┏') || data->game->board[waterRow - 1][waterColumns] == TEXT('┃') || data->game->board[waterRow - 1][waterColumns] == TEXT('E')) {
							piece = data->game->board[waterRow - 1][waterColumns];
							data->game->board[waterRow - 1][waterColumns] = TEXT('*');
							waterRow--;
							Sleep(2000);
							continue;
						}
						end = 1;
					}
					continue;
				}


			}
		}
	}

	data->game->shutdown = 1;

	if (end == 1 && win == 0) {
		_tprintf(TEXT("\n\nYOU LOST.\n\n"));
		showBoard(data);
		exit(1);
	}
	if (win == 1) {
		_tprintf(TEXT("\n\nYOU WON.\n\n"));
		data->game->board[data->game->endR][data->game->endC] = TEXT('E');
		showBoard(data);
		Sleep(3000);
		exit(1);
	}
}


DWORD WINAPI receiveCommnadsMonitor(LPVOID p) {
	ControlData* data = (ControlData*)p;
	Command command;
	int i = 0;


	do {
		WaitForSingleObject(data->commandEvent, INFINITE);
		WaitForSingleObject(data->commandMutex, INFINITE);
		if (i == BUFFERSIZE)
			i = 0;
		CopyMemory(&command, &(data->sharedMemCommand->commandMonitor[i]), sizeof(Command));
		i++;
		ReleaseMutex(data->commandMutex);

		if (command.command == 1) {
			setTime(data, command.parameter);
		}
		if (command.command == 2) {
			setWalls(data, command.parameter, command.paramter1);
		}
		if (command.command == 3) {
			setRandom(data);
			_tprintf(TEXT("\n\nRandom pieces = %s\n"), data->game->random ? TEXT("TRUE") : TEXT("FALSE"));
		}
	} while (data->game->shutdown != 1);


	return 1;

}

DWORD setTime(ControlData* data, DWORD time) {
	data->game->time = time;
	return 1;
}

DWORD setWalls(ControlData* data, DWORD row, DWORD column) {
	if ((row >= data->game->rows || row < 0) || (column >= data->game->rows || column < 0)) {
		_tprintf(TEXT("\nCant place a wall outside the board.\n"));
		return 0;
	}
	WaitForSingleObject(data->hMutex, INFINITE);
	data->game->board[row][column] = TEXT('P');
	ReleaseMutex(data->hMutex);
	return 1;

}

DWORD setRandom(ControlData* data) {
	if (data->game->random) {
		data->game->random = FALSE;
		return 1;
	}
	else
		data->game->random = TRUE;
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

	for (DWORD i = 0; i < BUFFERSIZE; i++) {
		p->sharedMemCommand->commandMonitor->parameter = 0;
		p->sharedMemCommand->commandMonitor->command = 0;
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
			randomRowE = (rand() % (data->game->rows / 2) + (data->game->rows / 2));
			randomColumnE = data->game->columns - 1;
		}
		else {
			randomColumnE = (rand() % (data->game->columns / 2) + (data->game->columns / 2));
			randomRowE = data->game->rows - 1;
		}
	}
	else if (quadrante == 2) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRowE = (rand() % (data->game->rows / 2) + (data->game->rows / 2));
			randomColumnE = 0;
		}
		else {
			randomColumnE = rand() % (data->game->columns / 2);
			randomRowE = data->game->rows - 1;
		}
	}
	else if (quadrante == 3) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRowE = rand() % (data->game->rows / 2);
			randomColumnE = data->game->columns - 1;
		}
		else {
			randomColumnE = (rand() % (data->game->columns / 2) + (data->game->columns / 2));
			randomRowE = 0;
		}
	}
	else if (quadrante == 4) {
		number = rand() % 2 + 1;
		if (number == 1) {
			randomRowE = rand() % (data->game->rows / 2);
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
	data->game->random = TRUE;
	data->game->index = 0;
	data->game->suspended = 0;
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
	threadData data;
	ControlData controlData;
	controlData.game = &game;
	controlData.data = &data;
	_tcscpy_s(registry.keyCompletePath, BUFFER, TEXT("SOFTWARE\\PipeGame\0"));
	TCHAR option[BUFFER];
	HANDLE hThreadSendDataToMonitor;
	HANDLE receiveCommandsMonitorThread;
	HANDLE hThreadTime;
	HANDLE waterFlowThread;
	HANDLE hPipe, hThread, hEventTemp;
	int i, numClientes = 0;
	controlData.game->shutdown = 0; // shutdown
	boolean firstTime = TRUE;
	DWORD offset, nBytes;
	srand((unsigned int)time(NULL));
	controlData.game->random = FALSE;



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
	}
	else {
		controlData.game->rows = _ttoi(argv[1]);
		controlData.game->columns = _ttoi(argv[2]);
		controlData.game->time = _ttoi(argv[3]);

		_tprintf(TEXT("\nBoard Loaded with [%d] rows and [%d] columns, water [%d] seconds.\n"), controlData.game->rows, controlData.game->columns, controlData.game->time);
	}

	controlData.data->terminar = 0;
	controlData.data->hMutex = CreateMutex(NULL, FALSE, NULL);
	if (controlData.data->hMutex == NULL) {
		_tprintf(_T("\n[ERRO] Criar Mutex! (CreateMutex)"));
		exit(-1);
	}

	initBoard(&controlData);
	for (i = 0; i < N; i++) {
		_tprintf(_T("[ESCRITOR] Criar uma cópia do pipe '%s'... (CreateNamedPipe)\n"), PIPE_NAME);
		hEventTemp = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEventTemp == NULL) {
			_tprintf(_T("[ERRO] Criar Evento! (CreateEvent)"));
			exit(-1);
		}
		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, N, 256 * sizeof(TCHAR), 256 * sizeof(TCHAR), 1000, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(_T("[ERRO] Criar NamedPipe! (CreateNamedPipe)"));
			exit(-1);
		}
		ZeroMemory(&controlData.data->hPipes[i].overlap, sizeof(controlData.data->hPipes[i].overlap));
		controlData.data->hPipes[i].hInstancia = hPipe;
		controlData.data->hPipes[i].overlap.hEvent = hEventTemp;
		controlData.data->hEvents[i] = hEventTemp;
		controlData.data->hPipes[i].activo = FALSE;

		if (ConnectNamedPipe(hPipe, &controlData.data->hPipes[i].overlap)) {
			_tprintf(_T("[ERRO] Ligação ao leitor! (ConnectNamedPipe)\n"));
			exit(-1);
		}
	}
	hThread = CreateThread(NULL, 0, ThreadMensagens, &controlData, 0, NULL);
	if (hThread == NULL) {
		_tprintf(_T("[ERRO] Criar Thread! (CreateThread)"));
		exit(-1);
	}


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

	/*while (_ttoi(option) != 3) {
		if (controlData.game->suspended == 1) {
			SuspendThread(hThreadTime);
			SuspendThread(waterFlowThread);
		}
		if (controlData.game->shutdown == 1)
			break;

		_tprintf(TEXT("\n1 - List Players(in development)."));
		_tprintf(TEXT("\n2 - Resume Game"));
		_tprintf(TEXT("\n3 - Quit\n\nCommand: "));

		_fgetts(option, BUFFER, stdin);

		switch (_ttoi(option)) {
		case 2:
			_tprintf(TEXT("\nGame resumed.\n"));
			controlData.game->suspended = 0;
			ResumeThread(hThreadTime);
			ResumeThread(waterFlowThread);
			play(&controlData);
			break;
		case 3:
			_tprintf(TEXT("\nClosing the application...\n"));
			controlData.game->shutdown = 1;
			break;
		case 5:
			if (firstTime) {
				ResumeThread(hThreadTime);
				play(&controlData);
			}
			else
				play(&controlData);
			break;
		default:
			_tprintf(TEXT("\nCouldn´t recognize the command.\n"));
			break;
		}
	}*/
	while (!controlData.data->terminar && numClientes < N) {
		_tprintf(_T("[ESCRITOR] Esperar ligação de um leitor...\n"));
		DWORD result = WaitForMultipleObjects(N, controlData.data->hEvents, FALSE, INFINITE);
		i = result - WAIT_OBJECT_0;
		if (i >= 0 && i < N) {
			_tprintf(_T("[ESCRITOR] Leitor %d chegou...\n"), i);
			ResumeThread(hThreadTime);
			if (GetOverlappedResult(controlData.data->hPipes[i].hInstancia, &controlData.data->hPipes[i].overlap, &nBytes, FALSE)) {
				ResetEvent(controlData.data->hEvents[i]);
				WaitForSingleObject(controlData.data->hMutex, INFINITE);
				controlData.data->hPipes[i].activo = TRUE;
				ReleaseMutex(controlData.data->hMutex);
			}
			numClientes++;
		}
	}
	for (i = 0; i < N; i++) {
		_tprintf(_T("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));

		if (!DisconnectNamedPipe(controlData.data->hPipes[i].hInstancia)) {
			_tprintf(_T("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
			exit(-1);
		}
		CloseHandle(controlData.data->hPipes[i].hInstancia);
	}
	// Waiting for the threads to end
	WaitForMultipleObjects(5, receiveCommandsMonitorThread, hThreadTime, hThreadSendDataToMonitor, waterFlowThread, hThread, TRUE, 2000);


	// Closing of all the handles
	RegCloseKey(registry.key);
	UnmapViewOfFile(controlData.sharedMemGame);
	UnmapViewOfFile(controlData.sharedMemCommand);
	CloseHandle(hThreadTime);
	CloseHandle(hThreadSendDataToMonitor);
	CloseHandle(receiveCommandsMonitorThread);
	CloseHandle(waterFlowThread);
	CloseHandle(controlData.commandMutex);
	CloseHandle(controlData.commandEvent);
	CloseHandle(controlData.hMapFileGame);
	CloseHandle(controlData.hMapFileMemory);
	CloseHandle(controlData.hMutex);
	CloseHandle(controlData.hWriteSem);
	CloseHandle(controlData.hReadSem);
	return 0;
}