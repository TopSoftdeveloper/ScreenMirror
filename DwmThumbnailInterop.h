#pragma once

// Ensure Windows Vista or later for DWM API
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <dwmapi.h>
#include <psapi.h>
#include <vector>
#include <string>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "psapi.lib")

// DWM_THUMBNAIL_PROPERTIES and related types are already defined in dwmapi.h
// DWM_TNP flags are also defined in dwmapi.h as enum or #defines
// We use the system definitions from the Windows SDK

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
