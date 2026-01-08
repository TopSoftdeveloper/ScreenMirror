#pragma once

#include "resource.h"

// Forward declarations for screen capture functions
BOOL GetFirstDisplayInfo(RECT* pRect);
BOOL CaptureScreen(HWND hWnd, HDC hdcDest, RECT* pSourceRect);