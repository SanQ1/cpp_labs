// client2.cpp : Лаб5 - Кл 2

#include "framework.h"
#include "client2.h"
#include <stdio.h>
#include <string.h>

#define MAX_LOADSTRING 100

static LPCWSTR MailslotName = L"\\\\.\\mailslot\\lab5";

#define ID_CLIENT_SENDMSG 40001

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

static HWND hwndEdit = nullptr;                 // вікно-журнал (тільки читання)

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

static void Log(const wchar_t* text);
static void BuildMessage(HWND hWnd, WCHAR* buf, int cch);
static void SendMsg(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT2, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT2));

    MSG msg;
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

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT2));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CLIENT2);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        1000, 520, 420, 300, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// Додає рядок у вікно-журнал.
static void Log(const wchar_t* text)
{
    int len = GetWindowTextLengthW(hwndEdit);
    SendMessageW(hwndEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

//
//  FUNCTION: BuildMessage()
//
//  PURPOSE: формує дані клієнта 2 (варіант 1).
//
static void BuildMessage(HWND hWnd, WCHAR* buf, int cch)
{
    // ширина та висота іконки додатку
    int iconW = GetSystemMetrics(SM_CXICON);
    int iconH = GetSystemMetrics(SM_CYICON);

    // ширина вертикальної полоси прокрутки
    int scrollW = GetSystemMetrics(SM_CXVSCROLL);

    // глибина кольору (біт на піксель)
    HDC hdc = GetDC(hWnd);
    int colorDepth = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(hWnd, hdc);

    swprintf_s(buf, cch,
        L"Дані Клієнта #2:\r\n"
        L" - ширина іконки (SM_CXICON) = %d px\r\n"
        L" - висота іконки (SM_CYICON) = %d px\r\n"
        L" - ширина вертикальної прокрутки (SM_CXVSCROLL) = %d px\r\n"
        L" - глибина кольору (BITSPIXEL) = %d біт",
        iconW, iconH, scrollW, colorDepth);
}

//
//  FUNCTION: SendMsg(HWND)
//
//  PURPOSE: відкриває поштову скриньку сервера і записує в неї повідомлення
//           (рядок UTF-16 з нулем, менше 256 символів - сервер читає 512 байт).
//
static void SendMsg(HWND hWnd)
{
    WCHAR szBuf[256];
    BuildMessage(hWnd, szBuf, 256);

    HANDLE hMailslot = CreateFileW(MailslotName, GENERIC_WRITE, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hMailslot == INVALID_HANDLE_VALUE)
    {
        WCHAR err[128];
        swprintf_s(err, 128, L"Помилка поштового серверу: %lu (сервер запущено? MailSlot -> Create)",
            GetLastError());
        Log(err);
        return;
    }

    DWORD cbWritten = 0;
    BOOL ok = WriteFile(hMailslot, szBuf, (DWORD)((wcslen(szBuf) + 1) * sizeof(WCHAR)),
        &cbWritten, NULL);
    CloseHandle(hMailslot);

    if (ok)
    {
        Log(L"");
        Log(L"Дані надіслані серверу:");
        Log(szBuf);
    }
    else
    {
        WCHAR err[128];
        swprintf_s(err, 128, L"Помилка надсилання повідомлення: %lu", GetLastError());
        Log(err);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hwndEdit = CreateWindowW(L"EDIT", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, 400, 200, hWnd, nullptr, hInst, nullptr);

        HMENU hMenu = GetMenu(hWnd);
        HMENU hClient = CreatePopupMenu();
        AppendMenuW(hClient, MF_STRING, ID_CLIENT_SENDMSG, L"Send message");
        InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hClient, L"&Client");
        DrawMenuBar(hWnd);

        Log(L"Клієнт 2 (поштова скринька). Client -> Send message");
    }
    break;
    case WM_SIZE:
        if (hwndEdit)
            MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
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
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        UNREFERENCED_PARAMETER(hdc);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

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