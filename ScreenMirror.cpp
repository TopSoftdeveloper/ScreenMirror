// ScreenMirror.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ScreenMirror.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
RECT g_firstDisplayRect = {0};                   // 1st display rectangle
BOOL g_bFirstDisplayFound = FALSE;               // Flag if 1st display found
UINT_PTR g_timerId = 0;                         // Timer ID for screen capture

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL                GetFirstDisplayInfo(RECT* pRect);
BOOL                CaptureScreen(HWND hWnd, HDC hdcDest, RECT* pSourceRect);

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
    LoadStringW(hInstance, IDC_SCREENMIRROR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SCREENMIRROR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;  // No menu
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

   // Get 1st display information
   if (!GetFirstDisplayInfo(&g_firstDisplayRect))
   {
       MessageBox(NULL, L"First display not found!", 
                  L"Error", MB_OK | MB_ICONERROR);
       return FALSE;
   }

   // Create window with title bar and borders (movable and resizable)
   // Start with a reasonable default size (e.g., 80% of display)
   int displayWidth = g_firstDisplayRect.right - g_firstDisplayRect.left;
   int displayHeight = g_firstDisplayRect.bottom - g_firstDisplayRect.top;
   int windowWidth = displayWidth * 8 / 10;
   int windowHeight = displayHeight * 8 / 10;
   
   // Center the window on the display
   int windowX = g_firstDisplayRect.left + (displayWidth - windowWidth) / 2;
   int windowY = g_firstDisplayRect.top + (displayHeight - windowHeight) / 2;
   
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
      WS_OVERLAPPEDWINDOW,  // Title bar, borders, and resize capability
      windowX, 
      windowY,
      windowWidth,
      windowHeight,
      nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // Start timer for real-time screen capture (30 FPS = ~33ms)
   g_timerId = SetTimer(hWnd, 1, 33, NULL);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_KEYDOWN  - Exit on ESC key
//  WM_TIMER    - Trigger screen capture update
//  WM_PAINT    - Capture and display 2nd display screen
//  WM_DESTROY  - Clean up and exit
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        {
            // Press ESC to exit fullscreen
            if (wParam == VK_ESCAPE)
            {
                DestroyWindow(hWnd);
            }
        }
        break;
    case WM_TIMER:
        {
            // Trigger repaint for screen capture
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            if (g_bFirstDisplayFound)
            {
                // Capture and display the entire 1st display screen
                // This captures the complete display area including all pixels
                CaptureScreen(hWnd, hdc, &g_firstDisplayRect);
            }
            else
            {
                // Fallback: show error message
                TextOut(hdc, 10, 10, L"First display not found!", 25);
            }
            
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        if (g_timerId != 0)
        {
            KillTimer(hWnd, g_timerId);
        }
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Callback function for EnumDisplayMonitors
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    static int monitorIndex = 0;
    RECT* pFirstDisplayRect = (RECT*)dwData;
    
    monitorIndex++;
    
    // Get the 1st monitor (first one found)
    if (monitorIndex == 1)
    {
        // Get complete monitor information including full screen area
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);
        
        if (GetMonitorInfo(hMonitor, &monitorInfo))
        {
            // Use rcMonitor to get the complete monitor rectangle (not just work area)
            *pFirstDisplayRect = monitorInfo.rcMonitor;
            g_bFirstDisplayFound = TRUE;
        }
        return FALSE; // Stop enumeration
    }
    
    return TRUE; // Continue enumeration
}

// Function to get 1st display monitor information
BOOL GetFirstDisplayInfo(RECT* pRect)
{
    if (pRect == NULL)
        return FALSE;
    
    g_bFirstDisplayFound = FALSE;
    
    // Enumerate all monitors
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)pRect);
    
    return g_bFirstDisplayFound;
}

// Function to capture screen from specified rectangle
BOOL CaptureScreen(HWND hWnd, HDC hdcDest, RECT* pSourceRect)
{
    if (pSourceRect == NULL || hWnd == NULL)
        return FALSE;
    
    // Calculate source dimensions - ensure we get the whole display
    int sourceWidth = pSourceRect->right - pSourceRect->left;
    int sourceHeight = pSourceRect->bottom - pSourceRect->top;
    
    if (sourceWidth <= 0 || sourceHeight <= 0)
        return FALSE;
    
    // Get screen DC for the entire virtual screen
    HDC hdcScreen = GetDC(NULL);
    if (hdcScreen == NULL)
        return FALSE;
    
    // Create compatible DC and bitmap with exact source dimensions
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (hdcMem == NULL)
    {
        ReleaseDC(NULL, hdcScreen);
        return FALSE;
    }
    
    // Create bitmap with the exact size of the source display
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, sourceWidth, sourceHeight);
    if (hBitmap == NULL)
    {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return FALSE;
    }
    
    // Select bitmap into memory DC
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    
    // Copy the entire screen content from the source rectangle
    // This captures the whole 2nd display area pixel by pixel
    BOOL bResult = BitBlt(hdcMem, 
                         0, 0,                    // Destination: start at top-left
                         sourceWidth, sourceHeight, // Full source dimensions
                         hdcScreen, 
                         pSourceRect->left,       // Source: X coordinate of 2nd display
                         pSourceRect->top,        // Source: Y coordinate of 2nd display
                         SRCCOPY);                // Copy entire area
    
    if (!bResult)
    {
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return FALSE;
    }
    
    // Get client rect for destination window
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int destWidth = clientRect.right - clientRect.left;
    int destHeight = clientRect.bottom - clientRect.top;
    
    // Stretch the captured bitmap to fit the window while maintaining aspect ratio
    // Use HALFTONE for better quality when stretching
    SetStretchBltMode(hdcDest, HALFTONE);
    SetBrushOrgEx(hdcDest, 0, 0, NULL);
    
    // Copy the entire captured content to the destination
    StretchBlt(hdcDest, 0, 0, destWidth, destHeight,
               hdcMem, 0, 0, sourceWidth, sourceHeight,
               SRCCOPY);
    
    // Cleanup
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    
    return TRUE;
}

