// lab3.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "lab3.h"
#include <vector>
#include <random>
#include <cstdlib>
#include <ctime>
#include <atomic>

#define MAX_LOADSTRING 100

struct ThreadParam {
    int x;  // top-left corner of the circle
    int y;
    int dx; // horizontal speed (pixels per tick)
    int dy; // vertical speed (pixels per tick)
};

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

std::vector<HANDLE>         g_threads;
std::vector<ThreadParam*>   g_params;
CRITICAL_SECTION            g_cs;

std::atomic<bool> g_stopThreads(false);
HWND        g_hWnd = nullptr;
const int   d = 100;  // diameter
const int   STEP = 5;    // speed in pixels per tick
const DWORD TICK_MS = 20;   // delay between moves


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI        ThreadProc(LPVOID lpParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    std::srand((unsigned)std::time(0));
    InitializeCriticalSection(&g_cs);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB3, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB3));

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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB3));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LAB3);
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
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    g_hWnd = hWnd;

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
//  WM_KEYDOWN  - spawn a new thread (and a new circle)
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - stop threads, clean up, post a quit message
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
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
    case WM_KEYDOWN:
    {
        // Ignore keyboard auto-repeat: one thread per physical key press
        if (lParam & (1 << 30))
            break;

        RECT rc;
        GetClientRect(hWnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        if (width <= d || height <= d)
            break;

        ThreadParam* newParam = new ThreadParam();

        // random start position
        newParam->x = rand() % (width - d);
        newParam->y = rand() % (height - d);

        // random movement: horizontal OR vertical, random direction
        int dir = (rand() % 2) ? 1 : -1;
        if (rand() % 2) {
            newParam->dx = dir * STEP;
            newParam->dy = 0;
        }
        else {
            newParam->dx = 0;
            newParam->dy = dir * STEP;
        }

        EnterCriticalSection(&g_cs);
        g_params.push_back(newParam);
        LeaveCriticalSection(&g_cs);

        HANDLE thread = CreateThread(nullptr, 0, ThreadProc, newParam, 0, nullptr);
        if (thread)
            g_threads.push_back(thread);

        InvalidateRect(hWnd, nullptr, FALSE);
    }
    break;
    case WM_ERASEBKGND:
        // Background is erased manually in WM_PAINT (double buffering) -> no flicker
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        // Draw everything into an off-screen bitmap, then copy it at once
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

        FillRect(memDC, &rc, (HBRUSH)(COLOR_WINDOW + 1));

        EnterCriticalSection(&g_cs);
        for (size_t i = 0; i < g_params.size(); ++i)
            Ellipse(memDC, g_params[i]->x, g_params[i]->y,
                g_params[i]->x + d, g_params[i]->y + d);
        LeaveCriticalSection(&g_cs);

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        g_stopThreads = true;

        // WaitForMultipleObjects accepts at most MAXIMUM_WAIT_OBJECTS (64) handles,
        // so wait in chunks
        for (size_t i = 0; i < g_threads.size(); i += MAXIMUM_WAIT_OBJECTS)
        {
            DWORD count = (DWORD)min((size_t)MAXIMUM_WAIT_OBJECTS, g_threads.size() - i);
            WaitForMultipleObjects(count, &g_threads[i], TRUE, 1000);
        }
        for (size_t i = 0; i < g_threads.size(); ++i)
            CloseHandle(g_threads[i]);

        for (size_t i = 0; i < g_params.size(); ++i)
            delete g_params[i];

        DeleteCriticalSection(&g_cs);

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

// Thread function: moves one circle horizontally or vertically
// and reverses the direction when the window border is reached.
DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    ThreadParam* param = static_cast<ThreadParam*>(lpParam);

    while (!g_stopThreads)
    {
        RECT rc;
        GetClientRect(g_hWnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        // Skip movement if the window is too small (e.g. minimized)
        if (width > d && height > d)
        {
            EnterCriticalSection(&g_cs);

            param->x += param->dx;
            param->y += param->dy;

            // horizontal bounce
            if (param->x < 0)
            {
                param->x = 0;
                param->dx = -param->dx;
            }
            else if (param->x + d > width)
            {
                param->x = width - d;
                param->dx = -param->dx;
            }

            // vertical bounce
            if (param->y < 0)
            {
                param->y = 0;
                param->dy = -param->dy;
            }
            else if (param->y + d > height)
            {
                param->y = height - d;
                param->dy = -param->dy;
            }

            LeaveCriticalSection(&g_cs);

            InvalidateRect(g_hWnd, nullptr, FALSE);
        }

        Sleep(TICK_MS);
    }

    return 0;
}