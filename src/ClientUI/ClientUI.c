#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "../Server/Game.h"
#include "Client.h"
#include "../Server/Pipes.h"
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
	unsigned int row = 50;
	unsigned int column = 50;
	TCHAR option[256];
	unsigned int number = 0;
	ClientData* data = (ClientData*)param;
	while (data->game->shutdown == 0) {
		ret = ReadFile(data->hPipe, data->game, sizeof(Game), &n, NULL);
		if(ret)
			_tprintf(_T("ALOALO"));
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
	HWND hWnd; // hWnd é o handler da janela, gerado mais abaixo por CreateWindow()
	HWND hButton;
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
	if (data.hPipe == NULL) {
		_tprintf(TEXT("Error connecting to the pipe (CreateFile) (%d)\n"), GetLastError());
		exit(-1);
	}
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
		CW_USEDEFAULT, // Largura da janela (em pixels)
		CW_USEDEFAULT, // Altura da janela (em pixels)
		(HWND)HWND_DESKTOP, // handle da janela pai (se se criar uma a partir de
		// outra) ou HWND_DESKTOP se a janela for a primeira, 
		// criada a partir do "desktop"
		(HMENU)NULL, // handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst, // handle da instância do programa actual ("hInst" é 
		// passado num dos parâmetros de WinMain()
		0); // Não há parâmetros adicionais para a janela
	HWND hwndButton = CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Pause!",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		650,         // x position 
		600,         // y position 
		100,        // Button width
		50,        // Button height
		hWnd,     // Parent window
		NULL,       // No menu.
		NULL,
		NULL);
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)&data);
		
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
	PAINTSTRUCT ps;
	static TCHAR c = TEXT('?');
	static BOOL flag = FALSE;
	static int totalPos = 0;
	ClientData* data;
	data = (ClientData*)GetWindowLongPtr(hWnd, 0);
	static HBITMAP hBmp[7];
	static BITMAP bmp;
	static HDC bmpDC = NULL;
	int Lx1=0, Lx2=0, Cy1=0, Cy2=0;

	switch (messg) {
	case WM_CHAR:
		c = (TCHAR)wParam;
	break;
	case WM_CLOSE:
		if(MessageBox(hWnd, _T("Tem a certeza que deseja sair?"), _T("Sair"), MB_YESNO) == IDYES)
			DestroyWindow(hWnd);
	break;
	case WM_LBUTTONDOWN:
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		hdc = GetDC(hWnd);
		GetClientRect(hWnd, &rect);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetBkMode(hdc, TRANSPARENT);
		rect.left = xPos;
		rect.top = yPos;
		DrawText(hdc, &c, 1, &rect, DT_SINGLELINE | DT_NOCLIP);
		ReleaseDC(hWnd, hdc);
	break;
	case WM_RBUTTONDOWN:
		flag = !flag;
		InvalidateRect(hWnd, NULL, TRUE);
		break;

	case WM_PAINT:
		if (flag)
		{
			hdc = BeginPaint(hWnd, &ps);
			int i = 0, j;

			Lx1 = 550, Lx2 = 570; // cell moves horizontally
			Cy1 = 150, Cy2 = 150;	// cell moves vertically
			for (j = 0; j < data->game->rows; j++)
			{
				for (i = 0; i < data->game->columns; i++)
				{
					Rectangle(hdc, Lx1, Cy1, Lx2, Cy2);
					Lx1 = Lx2;
					Lx2 = Lx2 + 20;
				}

				Cy1 = Cy2;
				Cy2 = Cy2 + 20;
				Lx1 = 550; Lx2 = 570;

			}
			EndPaint(hWnd, &ps);
		}
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