// ScreenMirror.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ScreenMirror.h"
#include "DwmThumbnailInterop.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HTHUMBNAIL g_hThumbnail = NULL;                 // DWM thumbnail handle
HWND g_hSourceWnd = NULL;                       // Source window handle
std::vector<CapturableWindow> g_windowsList;    // List of windows for picker dialog
HWND g_hSelectedWindow = NULL;                  // Selected window from picker dialog
BOOL g_bFullscreen = FALSE;                     // Flag for fullscreen state
RECT g_normalWindowRect = {0};                  // Store normal window position and size
DWORD g_normalWindowStyle = 0;                  // Store normal window style
RECT g_sourceRect = {0};                        // Source window region to capture (0 = use full window)
BOOL g_bUseSourceRect = FALSE;                  // Flag to use custom source rectangle
RECT g_hiddenWindowRect = {0};                  // Store window position when hidden (off-screen position)
BOOL g_bWindowHidden = FALSE;                   // Flag to track if window is hidden (moved off-screen)
HHOOK g_hKeyboardHook = NULL;                    // Global keyboard hook handle
HWND g_hMainWnd = NULL;                          // Main window handle for hook messages
#define WM_TOGGLE_VISIBILITY (WM_USER + 1)       // Custom message for toggle visibility

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                EnterFullscreen(HWND hWnd);
void                ExitFullscreen(HWND hWnd);
BOOL                GetFirstDisplayInfo(RECT* pRect);
HWND                FindGameExeWindow();
void                SetupThumbnail(HWND hWnd, HWND hSourceWnd);
LRESULT CALLBACK    LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
BOOL                InstallKeyboardHook();
void                UninstallKeyboardHook();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

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

    // Cleanup
    UninstallKeyboardHook();
    
    if (g_hThumbnail != NULL)
    {
        DwmUnregisterThumbnail(g_hThumbnail);
        g_hThumbnail = NULL;
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   // Create window
   int windowWidth = 800;
   int windowHeight = 600;
   
   // Create window without title bar (borderless window)
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
      WS_POPUP | WS_VISIBLE,  // No title bar, no borders
      CW_USEDEFAULT, 0,
      windowWidth,
      windowHeight,
      nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // Store main window handle for hook
   g_hMainWnd = hWnd;
   
   // Install global keyboard hook
   if (!InstallKeyboardHook())
   {
       // Hook installation failed, but continue anyway
   }

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            // First, try to auto-detect game.exe window with no title
            HWND hSelectedWnd = FindGameExeWindow();
            
            // If not found, show picker dialog
            if (hSelectedWnd == NULL)
            {
                if (!PickCaptureTarget(hWnd, &hSelectedWnd) || hSelectedWnd == NULL)
                {
                    // User cancelled or no window selected, close application
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                    break;
                }
            }
            
            // Setup thumbnail for selected window
            SetupThumbnail(hWnd, hSelectedWnd);
        }
        break;
    case WM_KEYDOWN:
        {
            // Press numpad 0 to enter/exit fullscreen
            if (wParam == VK_NUMPAD0)
            {
                if (g_bFullscreen)
                    ExitFullscreen(hWnd);
                else
                    EnterFullscreen(hWnd);
            }
        }
        break;
    case WM_TOGGLE_VISIBILITY:
        {
            // Toggle window hide/show by moving off-screen (triggered by global keyboard hook)
            if (g_bWindowHidden)
            {
                // Show: restore original position
                SetWindowPos(hWnd, NULL,
                           g_hiddenWindowRect.left,
                           g_hiddenWindowRect.top,
                           0, 0,
                           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                g_bWindowHidden = FALSE;
            }
            else
            {
                // Hide: store current position and move off-screen
                RECT currentRect;
                GetWindowRect(hWnd, &currentRect);
                
                // Store current position for restoration
                g_hiddenWindowRect = currentRect;
                
                // Get virtual screen dimensions (covers all monitors) to move window completely off-screen
                int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
                int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
                
                // Move window to off-screen position (outside all displays)
                // Use negative coordinates relative to virtual screen origin
                SetWindowPos(hWnd, NULL,
                           virtualLeft - virtualWidth - 100,
                           virtualTop - virtualHeight - 100,
                           0, 0,
                           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                g_bWindowHidden = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            // Update thumbnail properties when window size changes
            if (g_hThumbnail != NULL && g_hSourceWnd != NULL)
            {
                UpdateThumbnailProperties(hWnd, g_hThumbnail, g_hSourceWnd);
            }
        }
        break;
    case WM_DESTROY:
        {
            // Uninstall keyboard hook
            UninstallKeyboardHook();
            
            if (g_hThumbnail != NULL)
            {
                DwmUnregisterThumbnail(g_hThumbnail);
                g_hThumbnail = NULL;
            }
            PostQuitMessage(0);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Window enumeration callback
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    // Ignore invisible windows
    if (!IsWindowVisible(hWnd))
        return TRUE;

    // Get window title
    WCHAR szWindowText[1024] = {0};
    GetWindowTextW(hWnd, szWindowText, ARRAYSIZE(szWindowText) - 1);
    
    // Use process name as fallback if window has no title
    // (We'll set the title later after getting process info)

    // Ignore the picker dialog itself and its parent
    HWND hPickerDlg = (HWND)lParam;
    if (hWnd == hPickerDlg || hWnd == GetParent(hPickerDlg))
        return TRUE;

    // Get process information
    DWORD processId = 0;
    GetWindowThreadProcessId(hWnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
        return TRUE;

    WCHAR szProcessName[MAX_PATH] = {0};
    DWORD dwSize = ARRAYSIZE(szProcessName);
    if (QueryFullProcessImageNameW(hProcess, 0, szProcessName, &dwSize))
    {
        // Extract process name from full path
        WCHAR* pszName = wcsrchr(szProcessName, L'\\');
        if (pszName != NULL)
            pszName++;
        else
            pszName = szProcessName;

        // Ignore certain system processes
        WCHAR szLowerName[MAX_PATH] = {0};
        for (int i = 0; pszName[i]; i++)
            szLowerName[i] = towlower(pszName[i]);

        if (wcscmp(szLowerName, L"applicationframehost.exe") == 0 ||
            wcscmp(szLowerName, L"shellexperiencehost.exe") == 0 ||
            wcscmp(szLowerName, L"systemsettings.exe") == 0 ||
            wcscmp(szLowerName, L"winstore.app.exe") == 0 ||
            wcscmp(szLowerName, L"searchui.exe") == 0)
        {
            CloseHandle(hProcess);
            return TRUE;
        }

        // Get window client size (content area without borders/title bar)
        RECT clientRect;
        int clientWidth = 0;
        int clientHeight = 0;
        if (GetClientRect(hWnd, &clientRect))
        {
            clientWidth = clientRect.right - clientRect.left;
            clientHeight = clientRect.bottom - clientRect.top;
        }

        // Add window to list with size information
        CapturableWindow window;
        window.hWnd = hWnd;
        
        // Format: "Window Title (ProcessName.exe) [Width x Height]" - showing client size
        // If window has no title, use "(Untitled)" or process name
        std::wostringstream oss;
        if (wcslen(szWindowText) > 0)
        {
            oss << szWindowText << L" (" << pszName << L") [" << clientWidth << L" x " << clientHeight << L"]";
        }
        else
        {
            // Show as untitled window with process name
            oss << L"(Untitled - " << pszName << L") [" << clientWidth << L" x " << clientHeight << L"]";
        }
        window.name = oss.str();
        g_windowsList.push_back(window);
    }

    CloseHandle(hProcess);
    return TRUE;
}

// Get DPI scale factor for a window
FLOAT GetDpiScaleFactor(HWND hWnd)
{
    HDC hdc = GetDC(hWnd);
    if (hdc == NULL)
        return 1.0f;

    FLOAT dpi = (FLOAT)GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);
    
    return dpi / 96.0f;
}

// Update thumbnail properties
void UpdateThumbnailProperties(HWND hWnd, HTHUMBNAIL hThumbnail, HWND hSourceWnd)
{
    if (hThumbnail == NULL)
        return;

    FLOAT dpiScale = GetDpiScaleFactor(hWnd);
    
    // Get client size
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int width = (int)((clientRect.right - clientRect.left) * dpiScale);
    int height = (int)((clientRect.bottom - clientRect.top) * dpiScale);

    DWM_THUMBNAIL_PROPERTIES props = {0};
    props.dwFlags = DWM_TNP_VISIBLE | DWM_TNP_OPACITY | DWM_TNP_RECTDESTINATION | DWM_TNP_SOURCECLIENTAREAONLY;
    props.fVisible = TRUE;
    props.opacity = 255;
    props.fSourceClientAreaOnly = TRUE;
    props.rcDestination.left = 0;
    props.rcDestination.top = 0;
    props.rcDestination.right = width;
    props.rcDestination.bottom = height;

    // DwmUpdateThumbnailProperties expects const pointer
    HRESULT hr = DwmUpdateThumbnailProperties(hThumbnail, &props);
    if (FAILED(hr))
    {
        // Handle error if needed
    }
}

// Window picker dialog procedure
INT_PTR CALLBACK WindowPickerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Clear previous list
            g_windowsList.clear();
            
            // Get dialog handle for enumeration
            HWND hList = GetDlgItem(hDlg, IDC_LIST_WINDOWS);
            HWND hOwner = GetParent(hDlg);
            
            // Enumerate windows (pass dialog handle to ignore it)
            EnumWindows(EnumWindowsProc, (LPARAM)hDlg);
            
            // Populate list box
            for (size_t i = 0; i < g_windowsList.size(); i++)
            {
                int index = (int)SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)g_windowsList[i].name.c_str());
                SendMessageW(hList, LB_SETITEMDATA, index, (LPARAM)g_windowsList[i].hWnd);
            }
            
            // Select first item if available
            if (g_windowsList.size() > 0)
            {
                SendMessageW(hList, LB_SETCURSEL, 0, 0);
            }
            
            return (INT_PTR)TRUE;
        }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            
            if (wmId == IDOK)
            {
                HWND hList = GetDlgItem(hDlg, IDC_LIST_WINDOWS);
                int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
                
                if (sel != LB_ERR)
                {
                    g_hSelectedWindow = (HWND)SendMessageW(hList, LB_GETITEMDATA, sel, 0);
                    EndDialog(hDlg, IDOK);
                }
                else
                {
                    g_hSelectedWindow = NULL;
                    EndDialog(hDlg, IDCANCEL);
                }
                
                return (INT_PTR)TRUE;
            }
            
            if (wmId == IDC_LIST_WINDOWS && wmEvent == LBN_DBLCLK)
            {
                // Double-click on list item - treat as OK
                HWND hList = GetDlgItem(hDlg, IDC_LIST_WINDOWS);
                int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
                
                if (sel != LB_ERR)
                {
                    g_hSelectedWindow = (HWND)SendMessageW(hList, LB_GETITEMDATA, sel, 0);
                    EndDialog(hDlg, IDOK);
                }
                
                return (INT_PTR)TRUE;
            }
            
            if (wmId == IDCANCEL)
            {
                g_hSelectedWindow = NULL;
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Helper structure for game.exe window search
struct GameWindowSearchData {
    HWND hFoundWindow;
};

// Callback to find game.exe window with no title
BOOL CALLBACK FindGameExeWindowProc(HWND hWnd, LPARAM lParam)
{
    // Ignore invisible windows
    if (!IsWindowVisible(hWnd))
        return TRUE;

    // Get window title - we want windows with no title
    WCHAR szWindowText[1024] = {0};
    GetWindowTextW(hWnd, szWindowText, ARRAYSIZE(szWindowText) - 1);
    
    // Must have no title
    if (wcslen(szWindowText) > 0)
        return TRUE;

    // Get process information
    DWORD processId = 0;
    GetWindowThreadProcessId(hWnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
        return TRUE;

    WCHAR szProcessName[MAX_PATH] = {0};
    DWORD dwSize = ARRAYSIZE(szProcessName);
    BOOL found = FALSE;
    
    if (QueryFullProcessImageNameW(hProcess, 0, szProcessName, &dwSize))
    {
        // Extract process name from full path
        WCHAR* pszName = wcsrchr(szProcessName, L'\\');
        if (pszName != NULL)
            pszName++;
        else
            pszName = szProcessName;

        // Check if it's game.exe (case insensitive)
        WCHAR szLowerName[MAX_PATH] = {0};
        for (int i = 0; pszName[i]; i++)
            szLowerName[i] = towlower(pszName[i]);

        if (wcscmp(szLowerName, L"game.exe") == 0)
        {
            GameWindowSearchData* pData = (GameWindowSearchData*)lParam;
            if (pData != NULL)
            {
                pData->hFoundWindow = hWnd;
                found = TRUE;
            }
        }
    }

    CloseHandle(hProcess);
    return found ? FALSE : TRUE; // Stop enumeration if found
}

// Find game.exe window with no title
HWND FindGameExeWindow()
{
    GameWindowSearchData searchData;
    searchData.hFoundWindow = NULL;
    
    // Enumerate windows to find game.exe with no title
    EnumWindows(FindGameExeWindowProc, (LPARAM)&searchData);
    
    return searchData.hFoundWindow;
}

// Setup thumbnail for a source window
void SetupThumbnail(HWND hWnd, HWND hSourceWnd)
{
    if (hSourceWnd == NULL)
        return;
    
    g_hSourceWnd = hSourceWnd;
    
    // Get source window client size to match our window size
    RECT sourceClientRect;
    if (GetClientRect(hSourceWnd, &sourceClientRect))
    {
        // Use client size directly since our window is borderless (client = window)
        int clientWidth = sourceClientRect.right - sourceClientRect.left;
        int clientHeight = sourceClientRect.bottom - sourceClientRect.top;
        
        // Get current window position
        RECT currentRect;
        GetWindowRect(hWnd, &currentRect);
        
        // Resize window to match source window client size
        // Since our window is borderless (WS_POPUP), client size = window size
        SetWindowPos(hWnd, NULL, 
                   currentRect.left, 
                   currentRect.top,
                   clientWidth, 
                   clientHeight,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    
    // Register DWM thumbnail
    HRESULT hr = DwmRegisterThumbnail(hWnd, hSourceWnd, &g_hThumbnail);
    if (SUCCEEDED(hr))
    {
        UpdateThumbnailProperties(hWnd, g_hThumbnail, hSourceWnd);
    }
}

// Pick capture target window
BOOL PickCaptureTarget(HWND hOwnerWnd, HWND* phSelectedWnd)
{
    if (phSelectedWnd == NULL)
        return FALSE;

    *phSelectedWnd = NULL;
    g_hSelectedWindow = NULL;

    INT_PTR result = DialogBox(hInst, MAKEINTRESOURCE(IDD_WINDOWPICKER), hOwnerWnd, WindowPickerDlgProc);
    
    if (result == IDOK && g_hSelectedWindow != NULL)
    {
        *phSelectedWnd = g_hSelectedWindow;
        return TRUE;
    }

    return FALSE;
}

// Wrapper structure for monitor enumeration
struct MonitorEnumData {
    RECT* pRect;
    BOOL found;
};

// Monitor enumeration callback (internal version)
BOOL CALLBACK MonitorEnumProcInternal(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    UNREFERENCED_PARAMETER(hdcMonitor);
    UNREFERENCED_PARAMETER(lprcMonitor);
    
    MonitorEnumData* pData = (MonitorEnumData*)dwData;
    if (pData == NULL || pData->pRect == NULL)
        return FALSE;
    
    // Get complete monitor information including full screen area
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    
    if (GetMonitorInfo(hMonitor, &monitorInfo))
    {
        // Use rcMonitor to get the complete monitor rectangle (not just work area)
        *(pData->pRect) = monitorInfo.rcMonitor;
        pData->found = TRUE;
        return FALSE; // Stop enumeration after first monitor
    }
    
    return TRUE; // Continue enumeration
}

// Function to get 1st display monitor information
BOOL GetFirstDisplayInfo(RECT* pRect)
{
    if (pRect == NULL)
        return FALSE;
    
    ZeroMemory(pRect, sizeof(RECT));
    
    MonitorEnumData data;
    data.pRect = pRect;
    data.found = FALSE;
    
    // Enumerate all monitors
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProcInternal, (LPARAM)&data);
    
    // Check if we got a valid rect
    return data.found && (pRect->right > pRect->left && pRect->bottom > pRect->top);
}

// Function to enter fullscreen mode
void EnterFullscreen(HWND hWnd)
{
    if (g_bFullscreen)
        return; // Already in fullscreen
    
    // Store current window state
    GetWindowRect(hWnd, &g_normalWindowRect);
    g_normalWindowStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
    
    // Get display dimensions
    RECT displayRect;
    if (!GetFirstDisplayInfo(&displayRect))
    {
        // Fallback to primary monitor
        displayRect.left = 0;
        displayRect.top = 0;
        displayRect.right = GetSystemMetrics(SM_CXSCREEN);
        displayRect.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    
    int screenWidth = displayRect.right - displayRect.left;
    int screenHeight = displayRect.bottom - displayRect.top;
    
    // Remove window decorations
    SetWindowLongPtr(hWnd, GWL_STYLE, 
        WS_POPUP | WS_VISIBLE);
    
    // Set window to cover entire display
    SetWindowPos(hWnd, HWND_TOP,
        displayRect.left,
        displayRect.top,
        screenWidth,
        screenHeight,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    
    g_bFullscreen = TRUE;
    
    // Update thumbnail properties after fullscreen
    if (g_hThumbnail != NULL && g_hSourceWnd != NULL)
    {
        UpdateThumbnailProperties(hWnd, g_hThumbnail, g_hSourceWnd);
    }
}

// Function to exit fullscreen mode
void ExitFullscreen(HWND hWnd)
{
    if (!g_bFullscreen)
        return; // Already in windowed mode
    
    // Restore window style (borderless without title bar)
    SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    
    // Restore window position and size
    SetWindowPos(hWnd, HWND_TOP,
        g_normalWindowRect.left,
        g_normalWindowRect.top,
        g_normalWindowRect.right - g_normalWindowRect.left,
        g_normalWindowRect.bottom - g_normalWindowRect.top,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    
    g_bFullscreen = FALSE;
    
    // Update thumbnail properties after exiting fullscreen
    if (g_hThumbnail != NULL && g_hSourceWnd != NULL)
    {
        UpdateThumbnailProperties(hWnd, g_hThumbnail, g_hSourceWnd);
    }
}

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            KBDLLHOOKSTRUCT* pKbd = (KBDLLHOOKSTRUCT*)lParam;
            
            // Check if numpad 8 was pressed
            if (pKbd->vkCode == VK_NUMPAD8)
            {
                // Post custom message to main window to toggle visibility
                if (g_hMainWnd != NULL && IsWindow(g_hMainWnd))
                {
                    PostMessage(g_hMainWnd, WM_TOGGLE_VISIBILITY, 0, 0);
                }
            }
        }
    }
    
    // Call next hook
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// Install global keyboard hook
BOOL InstallKeyboardHook()
{
    if (g_hKeyboardHook != NULL)
        return TRUE; // Already installed
    
    // Install low-level keyboard hook
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, 
                                       LowLevelKeyboardProc, 
                                       hInst, 
                                       0);
    
    return (g_hKeyboardHook != NULL);
}

// Uninstall global keyboard hook
void UninstallKeyboardHook()
{
    if (g_hKeyboardHook != NULL)
    {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = NULL;
    }
}
