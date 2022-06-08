#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "../Server/ControlData.h"
#include "../Server/Pipes.h"

void showBoard(Game* data) {

	for (DWORD i = 0; i < data->rows; i++)
	{
		_tprintf(TEXT("\n"));
		for (DWORD j = 0; j < data->columns; j++)
			_tprintf(TEXT("%c "), data->board[i][j]);
	}
	_tprintf(TEXT("\n\n"));
	_tprintf(TEXT("\nPiece: %c\n\n"), data->piece);
}




int _tmain(int argc, LPTSTR argv[]) {
	Game game;
	HANDLE hPipe;
	int i = 0;
	BOOL ret;
	DWORD n;
	unsigned int row = 50;
	unsigned int column = 50;
	unsigned int number = 0;
	TCHAR option[256];
	game.shutdown = 0;

#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void)_setmode(_fileno(stdout), _O_WTEXT);
	(void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

	_tprintf(TEXT("\n-------------------CLIENT---------------\n"));
	if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("Error connecting to the pipe (WaitNamedPipe) (%d).\n"), GetLastError());
		exit(-1);
	}
	_tprintf(TEXT("\nLoading client...\n"));
	hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPipe == NULL) {
		_tprintf(TEXT("Error connecting to the pipe (CreateFile) (%d)\n"), GetLastError());
		exit(-1);
	}
	_tprintf(TEXT("\nConnected...\n"));

	while (game.shutdown == 0) {
		ret = ReadFile(hPipe, &game, sizeof(Game), &n, NULL);
		showBoard(&game);
		if (!ret || !n) {
			_tprintf(TEXT("\nGame ended...\n"));
			break;
		}
		_tprintf(TEXT("Received data...\n"));
		do {                                  
			while (row >= game.rows) {
				_tprintf(TEXT("\nRow: "));
				_fgetts(option, 256, stdin);
				row = _ttoi(option);

				if (row == -1) {
					game.suspended = 1;
					break;
				}

				if (row >= game.rows)
					_tprintf(TEXT("\nRows have to be between 0 and %d\n"), game.rows);
			}

			if (game.suspended == 1) {
				break;
			}

			while (column >= game.columns) {
				_tprintf(TEXT("\nColumn: "));
				_fgetts(option, 256, stdin);
				column = _ttoi(option);

				if (column == -1) {
					game.suspended = 1;
					break;
				}

				if (column >= game.columns)
					_tprintf(TEXT("\nColumn have to be between 0 and %d\n"), game.columns);
			}

			if (game.suspended == 1) {
				break;
			}

			if ((row == game.begginingR && column == game.begginingC) || (row == game.endR && column == game.endC)) {
				_tprintf(TEXT("\nYou cant play neither in the beginning or end position.\n"));
				row = 50;
				column = 50;
			}
		} while ((row == game.begginingR && column == game.begginingC) || (row == game.endR && column == game.endC));

		game.row = row;
		game.column = column;
		game.index++;
		row = 50;
		column = 50;
		if (!WriteFile(hPipe, &game, sizeof(Game), &n, NULL))
			_tprintf(_T("Error writting on the Pipe...\n"));
		else
			_tprintf(_T("\nSent Data...\n"));
	}
	CloseHandle(hPipe);
	Sleep(3000);
	return 0;
}