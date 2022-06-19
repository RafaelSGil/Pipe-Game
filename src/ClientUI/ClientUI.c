#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "../Server/Game.h"
#include "Client.h"
#include "../Server/Pipes.h"
#include "resource.h"
HANDLE hWnd;
/* ===================================================== */
/* Programa base (esqueleto) para aplicações Windows */
/* ===================================================== */
// Cria uma janela de nome "Janela Principal" e pinta fundo de branco
// Modelo para programas Windows:
// Composto por 2 funções: 
// WinMain() = Ponto de entrada dos programas windows
// 1) Define, cria e mostra a janela
// 2) Loop de recepção de mensagens provenientes do Windows
// TrataEventos()= Processamentos da janela (pode ter outro nome)
// 1) É chamada pelo Windows (callback) 
// 2) Executa código em função da mensagem recebida
DWORD WINAPI clientThread(LPVOID* param) {
	BOOL ret;
	DWORD n;
	ClientData* data = (ClientData*)param;
	while (data->game->shutdown == 0) {
			ReadFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
			WriteFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
	}
	return(1);
}

LRESULT CALLBACK HandleProcedures(HWND, UINT, WPARAM, LPARAM);


// Nome da classe da janela (para programas de uma só janela, normalmente este nome é 
// igual ao do próprio programa) "szprogName" é usado mais abaixo na definição das 
// propriedades do objecto janela
TCHAR szProgName[] = TEXT("Base");
// ============================================================================
// FUNÇÃO DE INÍCIO DO PROGRAMA: WinMain()
// ============================================================================
// Em Windows, o programa começa sempre a sua execução na função WinMain()que desempenha
// o papel da função main() do C em modo consola WINAPI indica o "tipo da função" (WINAPI
// para todas as declaradas nos headers do Windows e CALLBACK para as funções de
// processamento da janela)
// Parâmetros:
// hInst: Gerado pelo Windows, é o handle (número) da instância deste programa 
// hPrevInst: Gerado pelo Windows, é sempre NULL para o NT (era usado no Windows 3.1)
// lpCmdLine: Gerado pelo Windows, é um ponteiro para uma string terminada por 0
// destinada a conter parâmetros para o programa 
// nCmdShow: Parâmetro que especifica o modo de exibição da janela (usado em 
// ShowWindow()
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	MSG lpMsg; // MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp; // WNDCLASSEX é uma estrutura cujos membros servem para 
	Game game;
	ClientData data;
	HANDLE hThread;
	game.shutdown = 0;
	game.suspended = 0;
	data.game = &game;
	data.hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	
	hThread = CreateThread(NULL, 0, clientThread, &data, 0, NULL);
	if (hThread == NULL) {
		_tprintf(_T("\nError creating the thread for the client. (%d)"), GetLastError());
		exit(-1);
	}
	// definir as características da classe da janela
	// ============================================================================
		// 1. Definição das características da janela "wcApp" 
		// (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
		// ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX); // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst; // Instância da janela actualmente exibida 
	// ("hInst" é parâmetro de WinMain e vem 
	// inicializada daí)
	wcApp.lpszClassName = szProgName; // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = HandleProcedures; // Endereço da função de processamento da janela
	// ("TrataEventos" foi declarada no início e
	// encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW; // Estilo da janela: Fazer o redraw se for
	// modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(NULL, IDI_SHIELD); // "hIcon" = handler do ícon normal
	// "NULL" = Icon definido no Windows
	// "IDI_AP..." Ícone "aplicação"
	wcApp.hIconSm = LoadIcon(NULL, IDI_SHIELD); // "hIconSm" = handler do ícon pequeno
	// "NULL" = Icon definido no Windows
	// "IDI_INF..." Ícon de informação
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW); // "hCursor" = handler do cursor (rato) 
   // "NULL" = Forma definida no Windows
   // "IDC_ARROW" Aspecto "seta" 
	wcApp.lpszMenuName = NULL; // Classe do menu que a janela pode ter
   // (NULL = não tem menu)
	wcApp.cbClsExtra = 0; // Livre, para uso particular
	wcApp.cbWndExtra = sizeof(ClientData*); // Livre, para uso particular
	wcApp.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(175, 86, 162));
	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
	// "GetStockObject".Neste caso o fundo será branco
	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================
	if (!RegisterClassEx(&wcApp))
		return(0);
	// ============================================================================
	// 3. Criar a janela
	// ============================================================================
	hWnd = CreateWindow(
		szProgName, // Nome da janela (programa) definido acima
		TEXT("Client"), // Texto que figura na barra do título
		WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, // Estilo da janela (WS_OVERLAPPED= normal)
		CW_USEDEFAULT, // Posição x pixels (default=à direita da última)
		CW_USEDEFAULT, // Posição y pixels (default=abaixo da última)
		1600, // Largura da janela (em pixels)
		800, // Altura da janela (em pixels)
		(HWND)HWND_DESKTOP, // handle da janela pai (se se criar uma a partir de
		// outra) ou HWND_DESKTOP se a janela for a primeira, 
		// criada a partir do "desktop"
		(HMENU)NULL, // handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst, // handle da instância do programa actual ("hInst" é 
		// passado num dos parâmetros de WinMain()
		0); // Não há parâmetros adicionais para a janela
	data.hwnd = hWnd;
	SetWindowLongPtr(hWnd, 0, (LONG_PTR)&data);

	if (data.hPipe == INVALID_HANDLE_VALUE) {
		DestroyWindow(hWnd);
	}	
	HWND hwndButtonQuit = CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Quit",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		750,         // x position 
		600,         // y position 
		100,        // Button width
		75,        // Button height
		hWnd,     // Parent window
		(HMENU) BTN_QUIT,       // No menu.
		NULL,
		NULL);
	// ============================================================================
// 4. Mostrar a janela
// ============================================================================

	ShowWindow(hWnd, nCmdShow); // "hWnd"= handler da janela, devolvido por 
   // "CreateWindow"; "nCmdShow"= modo de exibição (p.e. 
   // normal/modal); é passado como parâmetro de WinMain()
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia à janela uma 

   // mensagem para pintar, mostrar dados, (refrescar)… 
   // ============================================================================
   // 5. Loop de Mensagens
   // ============================================================================
   // O Windows envia mensagens às janelas (programas). Estas mensagens ficam numa fila de
   // espera até que GetMessage(...) possa ler "a mensagem seguinte"
   // Parâmetros de "getMessage":
   // 1)"&lpMsg"=Endereço de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no 
   // início de WinMain()):
   // HWND hwnd handler da janela a que se destina a mensagem
   // UINT message Identificador da mensagem
   // WPARAM wParam Parâmetro, p.e. código da tecla premida
   // LPARAM lParam Parâmetro, p.e. se ALT também estava premida
   // DWORD time Hora a que a mensagem foi enviada pelo Windows
   // POINT pt Localização do mouse (x, y) 
   // 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
   // receber as mensagens para todas as janelas pertencentes à thread actual)
   // 3)Código limite inferior das mensagens que se pretendem receber
   // 4)Código limite superior das mensagens que se pretendem receber
   // NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
   // terminando então o loop de recepção de mensagens, e o programa 
	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg); // Pré-processamento da mensagem (p.e. obter código 
	   // ASCII da tecla premida)
		DispatchMessage(&lpMsg); // Enviar a mensagem traduzida de volta ao Windows, que
	   // aguarda até que a possa reenviar à função de 
	   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}
	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam); // Retorna sempre o parâmetro wParam da estrutura lpMsg
}
// ============================================================================
// FUNÇÃO DE PROCESSAMENTO DA JANELA
// Esta função pode ter um nome qualquer: Apenas é necesário que na inicialização da
// estrutura "wcApp", feita no início de WinMain(), se identifique essa função. Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pré-processadas
// no loop "while" da função WinMain()
// Parâmetros:
// hWnd O handler da janela, obtido no CreateWindow()
// messg Ponteiro para a estrutura mensagem (ver estrutura em 5. Loop...
// wParam O parâmetro wParam da estrutura messg (a mensagem)
// lParam O parâmetro lParam desta mesma estrutura
//
// NOTA:Estes parâmetros estão aqui acessíveis o que simplifica o acesso aos seus valores
//
// A função EndProc existe um "switch..." com "cases" que descriminam a mensagem
// recebida e a tratar. Estas mensagens são identificadas por constantes (p.e. 
// WM_DESTROY, WM_CHAR, WM_KEYDOWN, WM_PAINT...) definidas em windows.h
// ============================================================================

LRESULT CALLBACK HandleProcedures(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	DWORD xPos;
	DWORD yPos;
	RECT rect;
	HDC hdc;
	HDC hdcPiece = NULL;
	DWORD n;
	PAINTSTRUCT ps;
	DWORD totalOfPixels=0;
	static TCHAR c = TEXT('?');
	static BOOL flag = FALSE;
	static int totalPos = 0;
	ClientData* data;
	data = (ClientData*)GetWindowLongPtr(hWnd, 0);
	static HBITMAP hBmp;
	static HBITMAP hBmpBeggining;
	static HBITMAP hBmpEnd;
	static HBITMAP hBmpPieces[6];
	static BITMAP bmp = { 0 };
	static BITMAP bmpBeginning = { 0 };
	static BITMAP bmpEnd = { 0 };
	static BITMAP bmpPieces[6] = {0};
	static HDC bmpDC = NULL;
	static HDC bmpDCBeggining = NULL;
	static HDC bmpDCEnd = NULL;
	static HDC bmpDCPieces[6];
	static HDC hdcPieces[6] = { 0 };
	static int xBitmap;
	static int yBitmap;

	switch (messg) {
	case WM_CREATE:
		hBmp = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/boardCell.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		hBmpBeggining = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/posicao_inicio.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		hBmpEnd = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/posicao_fim.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		hBmpPieces[0] = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/tubo_horizontal.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		hBmpPieces[1] = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/tubo_vertical.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		hBmpPieces[2] = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/tubo_cima_direito.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		hBmpPieces[3] = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/tubo_cima_esquerdo.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		hBmpPieces[4] = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/tubo_baixo_esquerdo.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		hBmpPieces[5] = (HBITMAP)LoadImage(NULL, TEXT("../../bitmaps/tubo_baixo_direito.bmp"), IMAGE_BITMAP, 35, 35, LR_LOADFROMFILE);
		GetObject(hBmp, sizeof(bmp), &bmp);
		GetObject(hBmpBeggining, sizeof(bmpBeginning), &bmpBeginning);
		GetObject(hBmpEnd, sizeof(bmpEnd), &bmpEnd);
		for (int i = 0; i < 6; i++)
			GetObject(hBmpPieces[i], sizeof(bmpPieces[i]), &bmpPieces[i]);
		hdc = GetDC(hWnd);
		bmpDC = CreateCompatibleDC(hdc);
		bmpDCBeggining = CreateCompatibleDC(hdc);
		bmpDCEnd = CreateCompatibleDC(hdc);
		for (int i = 0; i < 6; i++)
			bmpDCPieces[i] = CreateCompatibleDC(hdc);
		SelectObject(bmpDC, hBmp);
		SelectObject(bmpDCBeggining, hBmpBeggining);
		SelectObject(bmpDCEnd, hBmpEnd);
		for (int i = 0; i < 6; i++)
			SelectObject(bmpDCPieces[i], hBmpPieces[i]);
		ReleaseDC(hWnd, hdc);
		GetClientRect(hWnd, &rect);
	break;
	case WM_COMMAND:
		if (LOWORD(wParam) == BTN_QUIT){
			data->game->shutdown = 1;
			Sleep(3000);
			DestroyWindow(hWnd);
		}
	break;
	case WM_CHAR:
		c = (TCHAR)wParam;
	break;
	case WM_CLOSE:
		if(MessageBox(hWnd, _T("Are you sure you want to leave?"), _T("Leave"), MB_YESNO) == IDYES)
			DestroyWindow(hWnd);
	break;
	case WM_LBUTTONDOWN:
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		hdc = GetDC(hWnd);
		if(data->game->piece == TEXT('━'))
			BitBlt(hdc, xPos, yPos, bmpPieces[0].bmHeight, bmpPieces[0].bmHeight, bmpDCPieces[0], 0, 0, SRCCOPY);
		else if(data->game->piece == TEXT('┃'))
			BitBlt(hdc, xPos, yPos, bmpPieces[1].bmHeight, bmpPieces[1].bmHeight, bmpDCPieces[1], 0, 0, SRCCOPY);
		else if(data->game->piece == TEXT('┏'))
			BitBlt(hdc, xPos, yPos, bmpPieces[2].bmHeight, bmpPieces[2].bmHeight, bmpDCPieces[2], 0, 0, SRCCOPY);
		else if(data->game->piece == TEXT('┓'))
			BitBlt(hdc, xPos, yPos, bmpPieces[3].bmHeight, bmpPieces[3].bmHeight, bmpDCPieces[3], 0, 0, SRCCOPY);
		else if(data->game->piece == TEXT('┛'))
			BitBlt(hdc, xPos, yPos, bmpPieces[4].bmHeight, bmpPieces[4].bmHeight, bmpDCPieces[4], 0, 0, SRCCOPY);
		else 
			BitBlt(hdc, xPos, yPos, bmpPieces[5].bmHeight, bmpPieces[5].bmHeight, bmpDCPieces[5], 0, 0, SRCCOPY);
		ReleaseDC(hWnd, hdc);
	break;
	case WM_RBUTTONDOWN:
		flag = !flag;
		InvalidateRect(hWnd, NULL, TRUE);
		break;

	case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rect);
			FillRect(hdc, &rect, CreateSolidBrush(RGB(255, 0, 0)));
			totalOfPixels = 35 * data->game->columns;		// colocar colunas que le do pipe no futuro
			xBitmap = (800 - (totalOfPixels / 2));
			yBitmap = 150;
			for (int j = 0; j < data->game->rows; j++) { // colocar linhas que le do pipe no futuro
				for (int i = 0; i < data->game->columns; i++) {	// colocar colunas que le do pipe no futuro
					if (j == data->game->begginingR && i == data->game->begginingC) {
							BitBlt(hdc, xBitmap, yBitmap, bmpBeginning.bmWidth, bmpBeginning.bmHeight, bmpDCBeggining, 0, 0, SRCCOPY);
					}
					else if (j == data->game->endR && i == data->game->endC) {
						BitBlt(hdc, xBitmap, yBitmap, bmpEnd.bmWidth, bmpEnd.bmHeight, bmpDCEnd, 0, 0, SRCCOPY);
					}
					else {
						BitBlt(hdc, xBitmap, yBitmap, bmp.bmWidth, bmp.bmHeight, bmpDC, 0, 0, SRCCOPY);
					}
					
					xBitmap = xBitmap + 35;                                                               
				}
				xBitmap = (800 - (totalOfPixels / 2));
				yBitmap = yBitmap + 35;
				
			}
			EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY: // Destruir a janela e terminar o programa 
	// "PostQuitMessage(Exit Status)"
		PostQuitMessage(0);
		break;
	default:
		// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar")
		// não é efectuado nenhum processamento, apenas se segue o "default" do Windows
		return(DefWindowProc(hWnd, messg, wParam, lParam));
		break; // break tecnicamente desnecessário por causa do return
	}
	return(0);
}