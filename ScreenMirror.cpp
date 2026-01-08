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

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

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

    // Cleanup thumbnail if exists
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
   
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
      WS_OVERLAPPEDWINDOW,
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
            // Pick window on creation
            HWND hSelectedWnd = NULL;
            if (PickCaptureTarget(hWnd, &hSelectedWnd) && hSelectedWnd != NULL)
            {
                g_hSourceWnd = hSelectedWnd;
                
                // Register DWM thumbnail
                HRESULT hr = DwmRegisterThumbnail(hWnd, hSelectedWnd, &g_hThumbnail);
                if (SUCCEEDED(hr))
                {
                    UpdateThumbnailProperties(hWnd, g_hThumbnail, hSelectedWnd);
                }
            }
            else
            {
                // User cancelled or no window selected, close application
                PostMessage(hWnd, WM_CLOSE, 0, 0);
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
    
    // Ignore untitled windows
    if (wcslen(szWindowText) == 0)
        return TRUE;

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

        // Add window to list
        CapturableWindow window;
        window.hWnd = hWnd;
        window.name = std::wstring(szWindowText) + L" (" + std::wstring(pszName) + L")";
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
