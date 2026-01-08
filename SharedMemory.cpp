#include "framework.h"
#include "SharedMemory.h"
#include <string.h>

HANDLE g_hSharedMapFile = NULL;
SharedMemoryData* g_sharedData = NULL;

// Open shared memory (this project opens, doesn't create)
void OpenSharedMemory()
{
    g_hSharedMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        "OneMonitorMemory");

    if (g_hSharedMapFile == NULL) {
        // Shared memory doesn't exist yet, will retry later
        return;
    }

    // Map a view of the file mapping into the address space of the current process
    g_sharedData = (SharedMemoryData*)MapViewOfFile(
        g_hSharedMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedMemoryData));

    if (g_sharedData == NULL) {
        CloseHandle(g_hSharedMapFile);
        g_hSharedMapFile = NULL;
    }
}

// Close shared memory
void CloseSharedMemory()
{
    if (g_sharedData != NULL) {
        UnmapViewOfFile(g_sharedData);
        g_sharedData = NULL;
    }
    
    if (g_hSharedMapFile != NULL) {
        CloseHandle(g_hSharedMapFile);
        g_hSharedMapFile = NULL;
    }
}

// Check for messages in shared memory
BOOL CheckSharedMemoryMessage(char* outMessage)
{
    if (g_sharedData == NULL) {
        // Try to open shared memory if not already open
        OpenSharedMemory();
        if (g_sharedData == NULL) {
            return FALSE;
        }
    }

    // Open mutex
    HANDLE hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, "OneMonitorMutex");
    if (hMutex == NULL) {
        return FALSE;
    }

    // Wait for mutex
    DWORD dwWaitResult = WaitForSingleObject(hMutex, 0); // Non-blocking
    if (dwWaitResult == WAIT_OBJECT_0) {
        // Check if there's a message
        if (g_sharedData->message[0] != '\0') {
            // Copy message
            strncpy_s(outMessage, MAX_MESSAGE_LENGTH, g_sharedData->message, _TRUNCATE);
            
            // Clear the message after reading
            g_sharedData->message[0] = '\0';
            
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
            return TRUE;
        }
        ReleaseMutex(hMutex);
    }
    
    CloseHandle(hMutex);
    return FALSE;
}

// Show window (move to original position)
void ShowWindowByMessage(HWND hWnd)
{
    // Access global variables from ScreenMirror.cpp
    extern RECT g_hiddenWindowRect;
    extern BOOL g_bWindowHidden;
    
    if (g_bWindowHidden) {
        // Restore original position
        SetWindowPos(hWnd, NULL,
                   g_hiddenWindowRect.left,
                   g_hiddenWindowRect.top,
                   0, 0,
                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        g_bWindowHidden = FALSE;
    }
}

// Hide window (move off-screen)
void HideWindowByMessage(HWND hWnd)
{
    // Access global variables from ScreenMirror.cpp
    extern RECT g_hiddenWindowRect;
    extern BOOL g_bWindowHidden;
    
    if (!g_bWindowHidden) {
        // Store current position
        RECT currentRect;
        GetWindowRect(hWnd, &currentRect);
        g_hiddenWindowRect = currentRect;
        
        // Get virtual screen dimensions to move window completely off-screen
        int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
        
        // Move window to off-screen position
        SetWindowPos(hWnd, NULL,
                   virtualLeft - virtualWidth - 100,
                   virtualTop - virtualHeight - 100,
                   0, 0,
                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        g_bWindowHidden = TRUE;
    }
}
