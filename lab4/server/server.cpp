// lab4.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "server.h"

#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "WS2_32.lib")

#define MAX_LOADSTRING 100

#define SERV_PORT 5000
#define WSA_ACCEPT (WM_USER+0)
#define WSA_NETEVENT (WM_USER+1)

char buff[512];

char mess[2048];
char* p_mess = mess;

HWND hWndEdit;
WSADATA wsaData;
WORD wVersionRequested = MAKEWORD(1, 1);

SOCKET srv_socket = INVALID_SOCKET;
SOCKADDR_IN srv_addr;
int ClientNum = -1;
SOCKET sock[2];
SOCKADDR_IN sockaddr[2];

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void ClientOff(HWND hWnd, int i);
void OnWSAAccept(HWND hWnd, LPARAM lParam);
void OnWSANetEvent(HWND hWnd, WPARAM wParam, LPARAM lParam);
void DisconnectClient(HWND hWnd, int i);
BOOL AcceptClient(HWND hWnd, int i);
void ServerStop(HWND hWnd);
void ServerStart(HWND hWnd);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SERVER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SERVER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_SERVER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SERVER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        100, 200, 420, 400, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hWndEdit = CreateWindow(TEXT("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 400, 300, hWnd, NULL, hInst, NULL);

        if (WSAStartup(wVersionRequested, &wsaData))
        {
            MessageBox(hWnd, L"WSAStartup Error", L"Error", MB_OK);
            return FALSE;
        }

        sprintf_s(p_mess, 100, "Статус: %s\r\n", wsaData.szSystemStatus);
        SendMessageA(hWndEdit, WM_SETTEXT, 0, (LPARAM)p_mess);
    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case ID_SERVER_START:
            ServerStart(hWnd);
            break;
        case ID_SERVER_STOP:
            ServerStop(hWnd);
            break;
        case ID_CLIENTOFF_1:
            ClientOff(hWnd, 0);
            break;
        case ID_CLIENTOFF_2:
            ClientOff(hWnd, 1);
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            WSACleanup();
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        WSACleanup();
        PostQuitMessage(0);
        break;
    case WSA_ACCEPT:
        OnWSAAccept(hWnd, lParam);
        break;
    case WSA_NETEVENT:
        OnWSANetEvent(hWnd, wParam, lParam);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


void ServerStart(HWND hWnd)
{
    if (srv_socket != INVALID_SOCKET)
    {
        MessageBox(hWnd, L"Сервер вже запущено.", L"Info", NULL);
        return;
    }

    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_socket == INVALID_SOCKET)
    {
        MessageBox(hWnd, L"Помилка створення сокета.", L"Error", MB_OK);
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERV_PORT);
    srv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv_socket, (LPSOCKADDR)&srv_addr, sizeof(srv_addr)) == SOCKET_ERROR)
    {
        closesocket(srv_socket);
        MessageBox(hWnd, L"Помилка зв'язування з портом", L"Error", MB_OK);
        return;
    }

    if (listen(srv_socket, 2) == SOCKET_ERROR)
    {
        closesocket(srv_socket);
        MessageBox(hWnd, L"Помилка очікування встановлення зв'язку.", L"Error", MB_OK);
        return;
    }

    if (WSAAsyncSelect(srv_socket, hWnd, WSA_ACCEPT, FD_ACCEPT))
    {
        closesocket(srv_socket);
        MessageBox(hWnd, L"WSAAsyncSelect error.", L"Error", MB_OK);
        return;
    }

    SendMessageA(hWndEdit, WM_SETTEXT, 0, (LPARAM)"Сервер запущено.");
}


void ServerStop(HWND hWnd)
{
    WSAAsyncSelect(srv_socket, hWnd, 0, 0);
    if (srv_socket != INVALID_SOCKET)
    {
        srv_socket = INVALID_SOCKET;
        closesocket(srv_socket);
        WSACleanup();
        return;
    }

    SendMessageA(hWndEdit, WM_SETTEXT, 0, (LPARAM)"Сервер зупинено.");
}


BOOL AcceptClient(HWND hWnd, int i)
{
    int addrlen = sizeof(sockaddr[i]);
    sock[i] = accept(srv_socket, (LPSOCKADDR)&sockaddr[i], (int FAR*) & addrlen);

    if (sock[i] != INVALID_SOCKET)
        if (!WSAAsyncSelect(sock[i], hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE))
            return TRUE;

    closesocket(sock[i]);
    return FALSE;
}


void DisconnectClient(HWND hWnd, int i)
{
    WSAAsyncSelect(sock[i], hWnd, 0, 0);
    closesocket(sock[i]);
}


void OnWSAAccept(HWND hWnd, LPARAM lParam)
{
    if (WSAGETSELECTERROR(lParam)) {
        MessageBox(hWnd, L"Accept error", L"Error", MB_OK);
        WSAAsyncSelect(srv_socket, hWnd, 0, 0);
        return;
    }

    if (ClientNum == 1) {
        MessageBox(hWnd, L"Число клієнтів більше ніж 2.", L"Error", MB_OK);
        return;
    }
    ++ClientNum;

    if (!AcceptClient(hWnd, ClientNum))
    {
        MessageBox(hWnd, L"Помилка з'єднання з клієнтом.", L"Error", MB_OK);
        return;
    }

    sprintf_s(buff, "Доданий клієнт %i\r\nАдреса: IP=%s Port=%u\r\n \0", ClientNum + 1,
        inet_ntoa(sockaddr[ClientNum].sin_addr), htons(sockaddr[ClientNum].sin_port));
    SendMessageA(hWndEdit, WM_SETTEXT, 0, (LPARAM)buff);
}


void OnWSANetEvent(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    char temp[256];
    char formatted_msg[512];

    int Number = -1;
    if      (sock[0] == (SOCKET)wParam) Number = 0;
    else if (sock[1] == (SOCKET)wParam) Number = 1;

    if (WSAGETSELECTEVENT(lParam) == FD_READ)
    {
        int rc = recv((SOCKET)wParam, temp, sizeof(temp) - 1, 0);
        if (rc > 0)
        {
            temp[rc] = '\0';
            sprintf_s(formatted_msg, sizeof(formatted_msg),
                "\r\n--- Дані від Клієнта %i ---\r\n%s\r\n",
                Number + 1, temp);
            strcat_s(mess, sizeof(mess), formatted_msg);
            SendMessageA(hWndEdit, WM_SETTEXT, 0, (LPARAM)p_mess);
        }
    }

    if (WSAGETSELECTEVENT(lParam) == FD_CLOSE)
    {
        if (Number != -1)
        {
            WSAAsyncSelect(sock[Number], hWnd, 0, 0);
            closesocket(sock[Number]);
            sprintf_s(temp, 30, "Клієнт %i завершив роботу", Number + 1);
            SendMessageA(hWndEdit, WM_SETTEXT, 0, (LPARAM)temp);
        }
    }
}

void ClientOff(HWND hWnd, int i)
{
    if (i > ClientNum) return;

    sprintf_s(buff, "Відключено клієнта %i", i + 1);
    SendMessageA(hWndEdit, WM_SETTEXT, 0, (LPARAM)buff);
    DisconnectClient(hWnd, i);

    --ClientNum;
}
