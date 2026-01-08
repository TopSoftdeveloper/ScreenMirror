#pragma once

#include <windows.h>
#include <dwmapi.h>
#include <psapi.h>
#include <vector>
#include <string>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "psapi.lib")

// DWM Thumbnail Properties flags (only define if not already defined)
#ifndef DWM_TNP_RECTDESTINATION
#define DWM_TNP_RECTDESTINATION 0x00000001
#define DWM_TNP_RECTSOURCE 0x00000002
#define DWM_TNP_OPACITY 0x00000004
#define DWM_TNP_VISIBLE 0x00000008
#define DWM_TNP_SOURCECLIENTAREAONLY 0x00000010
#endif

// DWM_THUMBNAIL_PROPERTIES is already defined in dwmapi.h, but if we need our own version:
#ifndef DWM_THUMBNAIL_PROPERTIES_DEFINED
// Note: The actual structure is already defined in dwmapi.h
// We're just ensuring we use the right type
#endif

// Capturable Window structure
struct CapturableWindow {
    HWND hWnd;
    std::wstring name;
};

// Forward declarations
BOOL PickCaptureTarget(HWND hOwnerWnd, HWND* phSelectedWnd);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WindowPickerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
void UpdateThumbnailProperties(HWND hWnd, HTHUMBNAIL hThumbnail, HWND hSourceWnd);
FLOAT GetDpiScaleFactor(HWND hWnd);
