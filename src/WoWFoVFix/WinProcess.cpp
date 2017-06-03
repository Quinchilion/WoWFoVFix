
#include "WinProcess.h"
#include "comdef.h"

//
//  SetPrivilege enables/disables process token privilege.
//
BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege) {
    LUID luid;
    BOOL bRet = FALSE;

    if (LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
        TOKEN_PRIVILEGES tp;

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;
        //
        //  Enable the privilege or disable all privileges.
        //
        if (AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
            //
            //  Check to see if you have proper access.
            //  You may get "ERROR_NOT_ALL_ASSIGNED".
            //
            bRet = (GetLastError() == ERROR_SUCCESS);
        }
    }
    return bRet;
}


std::string getLastErrorMessage() {
    DWORD errorCode = GetLastError();
    _com_error error(HRESULT_FROM_WIN32(errorCode));
    return std::string((char*)error.ErrorMessage()) + " [" + std::to_string(errorCode) + "]";
}

bool addDebugPrivileges() {
    bool success = false;
    HANDLE processHandle = GetCurrentProcess();
    HANDLE tokenHandle;

    if (OpenProcessToken(processHandle, TOKEN_ADJUST_PRIVILEGES, &tokenHandle)) {
        success = SetPrivilege(tokenHandle, SE_DEBUG_NAME, TRUE);
        CloseHandle(tokenHandle);
    }
    return success;
}

bool getWindowSize(HWND windowHandle, int& width, int& height) {
    RECT windowRect;
    if (!GetClientRect(windowHandle, &windowRect)) {
        return false;
    }
    width = windowRect.right - windowRect.left;
    height = windowRect.bottom - windowRect.top;
    return true;
}

bool createProcess(std::string executableName, HANDLE& processHandle) {
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));

    if (!CreateProcess(executableName.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo)) {
        return false;
    }

    processHandle = processInfo.hProcess;
    CloseHandle(processInfo.hThread);
    return true;
}

struct EnumWindowsCallbackData {
    DWORD processID;
    HWND mainWindowHandle = NULL;
};

BOOL CALLBACK enumWindowsCallback(HWND handle, LPARAM lParam) {
    auto data = reinterpret_cast<EnumWindowsCallbackData*>(lParam);

    DWORD windowProcessID;
    GetWindowThreadProcessId(handle, &windowProcessID);
    if (windowProcessID == data->processID && IsWindowVisible(handle)) {
        data->mainWindowHandle = handle;
        return FALSE;
    } else {
        return TRUE; // Continue iterating
    }

}

bool findWindowHandle(HANDLE processHandle, HWND& windowHandle) {
    EnumWindowsCallbackData callbackData;
    callbackData.processID = GetProcessId(processHandle);
    EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&callbackData));

    windowHandle = callbackData.mainWindowHandle;
    return windowHandle != NULL;
}

bool isProcessRunning(HANDLE processHandle) {
    DWORD exitCode = 0;
    GetExitCodeProcess(processHandle, &exitCode);
    return exitCode == STILL_ACTIVE;
}

void closeProcessHandle(HANDLE processHandle) {
    CloseHandle(processHandle);
}
