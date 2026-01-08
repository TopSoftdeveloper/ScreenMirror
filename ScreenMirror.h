#pragma once

#include "resource.h"

// Main window and dialog functions
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WindowPickerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL PickCaptureTarget(HWND hOwnerWnd, HWND* phSelectedWnd);
void UpdateThumbnailProperties(HWND hWnd, HTHUMBNAIL hThumbnail, HWND hSourceWnd);
FLOAT GetDpiScaleFactor(HWND hWnd);
