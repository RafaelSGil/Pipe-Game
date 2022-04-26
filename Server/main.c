#include <io.h>
#include <Windows.h>
#include <fcntl.h>
#include <stdio.h>	
#include <stdlib.h>
#include <tchar.h>
#define BUFFER 256
#define SIZE_DWORD 257*(sizeof(DWORD))



typedef struct _Game{
	TCHAR* board;
	DWORD rows;
	DWORD columns;
	DWORD time;
}Game;

typedef struct _Registry{
	HKEY key;
	TCHAR keyCompletePath[BUFFER];
	DWORD dposition;
	TCHAR name[BUFFER];
}Registry;


void initBoard(Game* game) {

	game->board = (TCHAR*)malloc(game->rows * game->columns * sizeof(TCHAR));

	if (game->board != NULL) {
		_tprintf(TEXT("\nMemory alocation for the board done successfully\n"));
	}
	for (DWORD i = 0; i < game->rows * game->columns; i++)
		_tcscpy_s(&game->board[i], sizeof(TCHAR), TEXT("_"));;
}
void showBoard(Game *game) {
	for (DWORD i = 0; i < game->rows; i++)
	{
		_tprintf(TEXT("\n"));
		for (DWORD j = 0; j < game->columns; j++)
			_tprintf(TEXT("%c "), game->board[i * game->rows + j]);
	}
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
	_tcscpy_s(registry.keyCompletePath, BUFFER, TEXT("SOFTWARE\\PipeGame\0"));

	if (argc != 4) {
		// Verifies if the key exists
		if (RegOpenKeyEx(HKEY_CURRENT_USER, registry.keyCompletePath, 0, KEY_ALL_ACCESS, &registry.key) != ERROR_SUCCESS) {
			// If doesnt exists creates the key
			if (RegCreateKeyEx(HKEY_CURRENT_USER, registry.keyCompletePath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &registry.key, &registry.dposition) == ERROR_SUCCESS)
			{
				_tprintf_s(TEXT("\nI just created the key for the PipeGame!"));
				DWORD rows = 10;
				DWORD columns = 10;
				DWORD time = 100;

				// Creates chain of values
				long rowsSet = RegSetValueEx(registry.key, TEXT("Rows"), 0, REG_DWORD, (LPBYTE)&rows, sizeof(DWORD));
				long columnsSet = RegSetValueEx(registry.key, TEXT("Columns"), 0, REG_DWORD, (LPBYTE)&columns, sizeof(DWORD));
				long timeSet = RegSetValueEx(registry.key, TEXT("Time"), 0, REG_DWORD, (LPBYTE)&time, sizeof(DWORD));

				if (rowsSet != ERROR_SUCCESS || columnsSet != ERROR_SUCCESS || timeSet != ERROR_SUCCESS) {
					_tprintf_s(TEXT("\nCouldnt configure the values for the pipe game!"));
					return -1;
				}
			}
		}
		DWORD sizeDword = SIZE_DWORD;
		// Querys the values for the variables
		long rowsRead = RegQueryValueEx(registry.key, TEXT("Rows"), NULL, NULL, (LPBYTE)&game.rows, &sizeDword);
		long columnsRead = RegQueryValueEx(registry.key, TEXT("Columns"), NULL, NULL, (LPBYTE)&game.columns, &sizeDword);
		long timeRead = RegQueryValueEx(registry.key, TEXT("Time"), NULL, NULL, (LPBYTE)&game.time, &sizeDword);
			
		if (rowsRead != ERROR_SUCCESS || columnsRead != ERROR_SUCCESS || timeRead != ERROR_SUCCESS) {
			_tprintf_s(TEXT("\nCouldnt read the values for the pipe game!"));
			return -1;
		}
	}else {
		game.rows = _ttoi(argv[1]);
		game.columns =  _ttoi(argv[2]);
		game.time =  _ttoi(argv[3]);
	}

	// Initializes the board for the game
	initBoard(&game);
	// Shows the board for the game
	showBoard(&game);

	// Frees the memory of the board
	free(*game.board);

	// Closes the key for the registry
	RegCloseKey(registry.key);

	return 0;
}