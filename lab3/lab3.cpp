// lab3.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "lab3.h"
#include <vector>
#include <random>
#include <cstdlib>

#define MAX_LOADSTRING 100

struct ThreadParam {
    int x;  // координати верхньої лівої точки кола
    int y;
    int dx; // швидкість переміщення по осі x
    int dy; // швидкість переміщення по осі y
};

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

std::vector<HANDLE>         g_threads;
std::vector<ThreadParam*>   g_params;
CRITICAL_SECTION            g_cs;

bool        g_stopThreads = false;
HWND        g_hWnd      = nullptr;
const int   d           = 100;  // diameter


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

    // TODO: Place code here.
    std::srand(std::time(0));
    InitializeCriticalSection(&g_cs);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB3, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
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

    return (int) msg.wParam;
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

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB3));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LAB3);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
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
            RECT rc;
            GetClientRect(hWnd, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            if (width <= d || height <= d)
                break;

            ThreadParam* newParam = new ThreadParam();
            newParam->x = rand() % (width - d);
            newParam->y = rand() % (height - d);

            EnterCriticalSection(&g_cs);
            g_params.push_back(newParam);
            LeaveCriticalSection(&g_cs);

            HANDLE thread = CreateThread(nullptr, 0, ThreadProc, newParam, 0, nullptr);
            if (thread)
                g_threads.push_back(thread);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            EnterCriticalSection(&g_cs);
            for(int i = 0; i < g_params.size(); ++i)
                Ellipse(hdc, g_params[i]->x, g_params[i]->y, g_params[i]->x + d, g_params[i]->y + d);
            LeaveCriticalSection(&g_cs);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        g_stopThreads = true;

        WaitForMultipleObjects((DWORD)(g_threads.size()), g_threads.data(), TRUE, 1000);
        for (int i = 0; i < g_threads.size(); ++i)
            CloseHandle(g_threads[i]);

        for (int i = 0; i < g_params.size(); ++i)
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

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    ThreadParam* param = static_cast<ThreadParam*>(lpParam);

    InvalidateRect(g_hWnd, 0, TRUE);
    while (!g_stopThreads)
    {
        Sleep(100);
    }

    return 0;
}