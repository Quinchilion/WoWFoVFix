// Defines a thin wrapper around various process and window related functions of
// the Windows API. This is meant to improve the readability of the main file.

#pragma once

#include <string>

#ifndef VC_EXTRALEAN
    // Exclude rarely-used stuff from Windows headers
    #define VC_EXTRALEAN 
#endif

#include <afx.h>
#include <afxwin.h> // MFC core and standard components

// Returns a formatted message with the last error
std::string getLastErrorMessage();

// Adds debug privileges to the current process. Returns true if successful.
bool addDebugPrivileges();

// Retrieves the size of the given window. Returns true if successful.
bool getWindowSize(HWND windowHandle, int& width, int& height);

// Runs the given executable and keeps its process handle. Returns true if successful.
bool createProcess(std::string executableName, HANDLE& processHandle);

// From the given processID, finds its main window handle. Returns true if successful.
bool findWindowHandle(HANDLE processHandle, HWND& windowHandle);

// Returns true if process is still running
bool isProcessRunning(HANDLE processHandle);

// Reads process memory at the given address and fetches the value. Returns true if successful.
template <typename T>
bool readMemoryValue(HANDLE processHandle, void* address, T& value) {
    return ReadProcessMemory(processHandle, address, &value, sizeof(T), NULL);
}

// Writes the value to the process memory at the given address. Returns true if successful.
template <typename T>
bool writeMemoryValue(HANDLE processHandle, void* address, T value) {
    return WriteProcessMemory(processHandle, address, &value, sizeof(T), NULL);
}

// Closes a previously opened process handle
void closeProcessHandle(HANDLE processHandle);