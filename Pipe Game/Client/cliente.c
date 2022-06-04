#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "../../Server/Pipes.h"
#include "../../Server/ControlData.h"
void showBoard(Game* data) {
    _tprintf(TEXT("\n\nTime: [%d]\n\n"), data->time);
    for (DWORD i = 0; i < data->rows; i++)
    {
        _tprintf(TEXT("\n"));
        for (DWORD j = 0; j < data->columns; j++)
            _tprintf(TEXT("%c "), data->board[i][j]);
    }
    _tprintf(TEXT("\n\n"));
}

int _tmain(int argc, LPTSTR argv[]) {
    Game game;
    HANDLE hPipe;
    int i = 0;
    BOOL ret;
    DWORD n;

#ifdef UNICODE
    (void)_setmode(_fileno(stdin), _O_WTEXT);
    (void)_setmode(_fileno(stdout), _O_WTEXT);
    (void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

    _tprintf(TEXT("[LEITOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n"),
        PIPE_NAME);
    if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
        exit(-1);
    }
    _tprintf(TEXT("[LEITOR] Ligação ao pipe do escritor... (CreateFile)\n"));
    hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hPipe == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
        exit(-1);
    }
    _tprintf(TEXT("[LEITOR] Liguei-me...\n"));

    while (1) {
        ret = ReadFile(hPipe, &game, sizeof(Game), &n, NULL);
        showBoard(&game);
        Sleep(5000);
        if (!ret || !n) {
            _tprintf(TEXT("[LEITOR] %d %d... (ReadFile)\n"), ret, n);
            break;
        }
        _tprintf(TEXT("[LEITOR] Recebi %d bytes\n"), n);
        if (!WriteFile(hPipe, &game, sizeof(Game), &n, NULL))
            _tprintf(_T("[ERRO] Escrever no pipe! (WriteFile)\n"));
        else
            _tprintf(_T("[LEITOR] Enviei %d bytes ao escritor [%d]... (WriteFile)\n"), n, i);
    }
    CloseHandle(hPipe);
    Sleep(200);
    return 0;
}