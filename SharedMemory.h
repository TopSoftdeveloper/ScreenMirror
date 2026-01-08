#pragma once
#include <Windows.h>

#define MAX_MESSAGE_LENGTH 32

typedef struct {
    char message[MAX_MESSAGE_LENGTH];  // Message buffer for "SHOW2ND" or "HIDE2ND"
    HANDLE mutex;                       // Mutex handle
} SharedMemoryData;

void OpenSharedMemory();
void CloseSharedMemory();
BOOL CheckSharedMemoryMessage(char* outMessage);
void ShowWindowByMessage(HWND hWnd);
void HideWindowByMessage(HWND hWnd);
