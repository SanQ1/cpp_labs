// client1.cpp : Client 1 (variant 1) - sends system information to the server over a TCP socket.
//
// Data sent:
//   - menu bar height          GetSystemMetrics(SM_CYMENU)
//   - mouse presence           GetSystemMetrics(SM_MOUSEPRESENT)
//   - screen width             GetDeviceCaps(hdc, HORZRES)

// WSAAsyncSelect is marked deprecated (C4996); the lab uses it on purpose
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// winsock2.h must come before windows.h (which framework.h includes)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "framework.h"
#include "client1.h"

#pragma comment(lib, "Ws2_32.lib")

#define MAX_LOADSTRING 100

#define SERV_PORT               5000
#define SERV_HOST               "127.0.0.1"     // server address (the server runs on the same PC)
#define WSA_NETEVENT            (WM_USER + 1)   // socket event message

// Menu command IDs of the "Client" menu (the menu is created in code, see WM_CREATE)
#define ID_CLIENT_SETCONNECTION 40001
#define ID_CLIENT_SENDMSG       40002

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// client-server variables
static HWND   hwndEdit = nullptr;               // read-only log window
static SOCKET cln_socket = INVALID_SOCKET;      // client socket
static WSADATA wsaData;                         // Windows Sockets implementation info

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

static void Log(const wchar_t* text);
static void LogA(const char* text);
static BOOL SetConnection(HWND hWnd);
static void SendMsg(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT1));

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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CLIENT1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        1000, 200, 420, 300, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// Appends a line to the log window (wide string).
static void Log(const wchar_t* text)
{
    int len = GetWindowTextLengthW(hwndEdit);
    SendMessageW(hwndEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

// Appends a line to the log window (narrow string, e.g. data received/sent).
static void LogA(const char* text)
{
    wchar_t wbuf[1100];
    int n = MultiByteToWideChar(CP_ACP, 0, text, -1, wbuf, (int)(sizeof(wbuf) / sizeof(wbuf[0])));
    if (n > 0)
        Log(wbuf);
}

//
//  FUNCTION: SetConnection(HWND)
//
//  PURPOSE: Creates the socket and connects to the server.
//
static BOOL SetConnection(HWND hWnd)
{
    if (cln_socket != INVALID_SOCKET)
    {
        Log(L"З'єднання вже встановлено.");
        return TRUE;
    }

    // create the socket
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
    {
        MessageBoxW(hWnd, L"Помилка створення сокета", L"Error", MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    // server address and port
    sockaddr_in dest_sin = {};
    dest_sin.sin_family = AF_INET;
    dest_sin.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, SERV_HOST, &dest_sin.sin_addr) != 1)
    {
        closesocket(s);
        MessageBoxW(hWnd, L"Адреса хоста не визначена", L"Error", MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    // establish the connection
    if (connect(s, (SOCKADDR*)&dest_sin, sizeof(dest_sin)) == SOCKET_ERROR)
    {
        closesocket(s);
        MessageBoxW(hWnd, L"Помилка з'єднання. Чи запущено сервер (Server -> Start)?",
            L"Error", MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    // from now on the window gets WSA_NETEVENT when data arrives or the server closes the connection
    if (WSAAsyncSelect(s, hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE))
    {
        closesocket(s);
        MessageBoxW(hWnd, L"WSAAsyncSelect error", L"Error", MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    cln_socket = s;
    Log(L"Зв'язок встановлено!");
    return TRUE;
}

//
//  FUNCTION: SendMsg(HWND)
//
//  PURPOSE: Collects the data of variant 1 (client 1) and sends it to the server.
//
static void SendMsg(HWND hWnd)
{
    if (cln_socket == INVALID_SOCKET)
    {
        Log(L"Спочатку встановіть з'єднання (Client -> Set connection).");
        return;
    }

    // menu bar height
    int menuHeight = GetSystemMetrics(SM_CYMENU);

    // mouse presence
    bool mousePresent = GetSystemMetrics(SM_MOUSEPRESENT) != 0;

    // screen width in pixels
    HDC hdc = GetDC(hWnd);
    int screenWidth = GetDeviceCaps(hdc, HORZRES);
    ReleaseDC(hWnd, hdc);

    char szBuf[256];
    sprintf_s(szBuf, sizeof(szBuf),
        "Menu bar height (SM_CYMENU): %d px\r\n"
        "Mouse: %s\r\n"
        "Screen width (HORZRES): %d px",
        menuHeight,
        mousePresent ? "connected" : "not connected",
        screenWidth);

    if (send(cln_socket, szBuf, (int)strlen(szBuf), 0) != SOCKET_ERROR)
    {
        Log(L"");
        Log(L"Дані надіслані серверу:");
        LogA(szBuf);
    }
    else
    {
        Log(L"Помилка надсилання повідомлення");
    }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_CREATE       - create the log window, init Winsock, add the "Client" menu
//  WM_SIZE         - resize the log window
//  WM_COMMAND      - process the application menu
//  WSA_NETEVENT    - data from the server / connection closed
//  WM_DESTROY      - close the socket, post a quit message and return
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hwndEdit = CreateWindowW(L"EDIT", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, 400, 200, hWnd, nullptr, hInst, nullptr);

        // add the "Client" popup menu after "File"
        HMENU hMenu = GetMenu(hWnd);
        HMENU hClient = CreatePopupMenu();
        AppendMenuW(hClient, MF_STRING, ID_CLIENT_SETCONNECTION, L"Set connection");
        AppendMenuW(hClient, MF_STRING, ID_CLIENT_SENDMSG, L"Send message");
        InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hClient, L"&Client");
        DrawMenuBar(hWnd);

        // WSAStartup lets the program specify the required Winsock version
        int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (err)
        {
            MessageBoxW(hWnd, L"WSAStartup Error", L"ERROR", MB_OK | MB_ICONSTOP);
            return -1;
        }

        wchar_t szInfo[512];
        swprintf_s(szInfo, L"Використовується %hs\r\nСтатус: %hs",
            wsaData.szDescription, wsaData.szSystemStatus);
        Log(szInfo);
    }
    break;
    case WM_SIZE:
        if (hwndEdit)
            MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case ID_CLIENT_SETCONNECTION:
            SetConnection(hWnd);
            break;
        case ID_CLIENT_SENDMSG:
            SendMsg(hWnd);
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WSA_NETEVENT:
    {
        // data from the server: receive and show it
        if (WSAGETSELECTEVENT(lParam) == FD_READ)
        {
            char szRecv[1024];
            int rc = recv((SOCKET)wParam, szRecv, sizeof(szRecv) - 1, 0);
            if (rc > 0)
            {
                szRecv[rc] = '\0';
                Log(L"");
                Log(L"Дані від сервера:");
                LogA(szRecv);
            }
        }
        // the connection was closed (server stopped or "Client off")
        if (WSAGETSELECTEVENT(lParam) == FD_CLOSE)
        {
            if (cln_socket != INVALID_SOCKET)
            {
                WSAAsyncSelect(cln_socket, hWnd, 0, 0);
                closesocket(cln_socket);
                cln_socket = INVALID_SOCKET;
            }
            Log(L"З'єднання з сервером закрито.");
            MessageBoxW(hWnd, L"Сервер закритий", L"Server", MB_OK);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        UNREFERENCED_PARAMETER(hdc);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        if (cln_socket != INVALID_SOCKET)
        {
            WSAAsyncSelect(cln_socket, hWnd, 0, 0);
            closesocket(cln_socket);
            cln_socket = INVALID_SOCKET;
        }
        WSACleanup();
        PostQuitMessage(0);
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