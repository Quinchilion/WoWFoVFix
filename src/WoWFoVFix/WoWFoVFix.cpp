
#include "WinProcess.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <fstream>

double RADIANS_TO_DEGREES = 180.0 / M_PI;
double DEGREES_TO_RADIANS = 1.0 / RADIANS_TO_DEGREES;

const std::string WOW_EXECUTABLE_PATH = "Wow.exe";
const uint32_t INSTANCE_POINTER_MEMORY_OFFSET = 0xC6ECCC;
const uint32_t CAMERA_POINTER_MEMORY_OFFSET = 0x732C;
const uint32_t FIELD_OF_VIEW_MEMORY_OFFSET = 0x40;

const std::string LOG_FILE_NAME = "WoWFoVFix.log";
const int WINDOW_OPEN_WAIT_MILLISECONDS = 250;
const int MAINTAIN_LOOP_WAIT_MILLISECONDS = 250;

// Default FoV value for the TBC client
const float DEFAULT_WOW_FIELD_OF_VIEW = static_cast<float>(DEGREES_TO_RADIANS * 90.0);

// Base FoV value as used in WotLK for a 16/9 screen ratio
const double BASE_FIELD_OF_VIEW = 1.925;
const double BASE_SCREEN_RATIO = 16.0 / 9.0;
const double BASE_VIEW_DISTANCE = BASE_SCREEN_RATIO / std::tan(BASE_FIELD_OF_VIEW / 2.0);

const double MAX_FIELD_OF_VIEW = DEGREES_TO_RADIANS * 170.0;
const double MIN_FIELD_OF_VIEW = DEGREES_TO_RADIANS * 30.0;

// Travels through the memory space of the process to find the address for the field of view value
// used by the in-game camera.
void* getFieldOfViewAddress(HANDLE processHandle) {
    uint32_t cameraPointerAddress;
    if (!readMemoryValue(processHandle, (void*)INSTANCE_POINTER_MEMORY_OFFSET, cameraPointerAddress)) {
        return nullptr;
    }

    uint32_t cameraAddress;
    if (!readMemoryValue(processHandle, (void*)(cameraPointerAddress + CAMERA_POINTER_MEMORY_OFFSET), cameraAddress)) {
        return nullptr;
    }

    return (void*)(cameraAddress + FIELD_OF_VIEW_MEMORY_OFFSET);
}

// Returns the field of view value found by rescaling the game's expected screen ratio to the actual screen ratio.
float calculateOptimalFov(int screenWidth, int screenHeight) {
    if (screenWidth == 0 || screenHeight == 0) {
        return DEFAULT_WOW_FIELD_OF_VIEW;    
    }

    double screenRatio = screenWidth / static_cast<double>(screenHeight);

    // Rescale from base viewing distance to the new screen ratio
    double newFov = 2.0 * std::atan(screenRatio / BASE_VIEW_DISTANCE);

    // Clamp field of view to the valid range to prevent any weird stuff from happening
    if (newFov > MAX_FIELD_OF_VIEW) newFov = MAX_FIELD_OF_VIEW;
    if (newFov < MIN_FIELD_OF_VIEW) newFov = MIN_FIELD_OF_VIEW;

    return static_cast<float>(newFov);
}

// Waits until a valid window for the process exists. Returns true if successful.
bool waitForWindow(HANDLE processHandle, HWND& windowHandle) {
    windowHandle = NULL;
    findWindowHandle(processHandle, windowHandle);

    while (windowHandle == NULL) {
        Sleep(WINDOW_OPEN_WAIT_MILLISECONDS);

        if (!isProcessRunning(processHandle)) {
            std::cerr << "WoW process was closed while waiting for its window." << std::endl;
            return false;
        }

        findWindowHandle(processHandle, windowHandle);
    }

    return true;
}

// The main application loop. It periodically checks for the field of view value in the process' memory
// and changes it to match the current window size of the game.
bool fovMaintainLoop(HANDLE processHandle) {
    HWND windowHandle = NULL;
    if (!waitForWindow(processHandle, windowHandle)) return false;

    int width = 0, height = 0;
    float currentFovValue = DEFAULT_WOW_FIELD_OF_VIEW;

    while (isProcessRunning(processHandle)) {
        Sleep(MAINTAIN_LOOP_WAIT_MILLISECONDS);

        // Try to fetch the currently set field of view value. If this fails, a camera is probably not yet
        // created in the game. This can happen when the user is not yet logged in.
        void* processFovAddress = getFieldOfViewAddress(processHandle);
        if (processFovAddress == nullptr) {
            continue;
        }

        float processFovValue;
        if (!readMemoryValue(processHandle, processFovAddress, processFovValue)) {
            continue;
        }

        // Check if the value was changed by the game. This can be caused by relogging or reloading the ui.
        bool needsUpdate = false;
        if (processFovValue != currentFovValue) {
            if (processFovValue != DEFAULT_WOW_FIELD_OF_VIEW) {
                // The game's field of view has an unexpected value. This most likely means an unintended memory
                // address was accessed and writing to this value would have an undefined behavior.
                std::cerr << "The game's field of view setting has an unexpected value: " << processFovValue << std::endl;
                std::cerr << "Exiting to prevent possible undefined behavior." << std::endl;
                return false;
            }
            needsUpdate = true;
        }

        // See if the window size has changed
        int newWidth, newHeight = 0;
        if (!getWindowSize(windowHandle, newWidth, newHeight)) {
            // Unable to retrieve the size of the game window. It's possible the handle is no longer valid.
            // So let's get a new one.
            if (!waitForWindow(processHandle, windowHandle)) return false;
            continue;
        }

        if (newWidth != width || newHeight != height) {
            width = newWidth; height = newHeight;
            needsUpdate = true;
        }

        // Finally, update the game's field of view value
        if (needsUpdate) {
            currentFovValue = calculateOptimalFov(width, height);

            // Edit the field of view value in the game's memory
            if (!writeMemoryValue(processHandle, processFovAddress, currentFovValue)) {
                std::cerr << "Failed to write a new field of view value: " << getLastErrorMessage() << std::endl;
                return false;
            }

            std::cout
                << "Changing FoV to: " << currentFovValue * RADIANS_TO_DEGREES 
                << " (window size " << width << "x" << height << ")" << std::endl;
        }
    }

    return true;
}

void main() {
    // Open and redirect stdout & stderr to a log file
    std::ofstream logStream(LOG_FILE_NAME, std::ios_base::trunc);
    std::cout.rdbuf(logStream.rdbuf());
    std::cerr.rdbuf(logStream.rdbuf());

    if (!addDebugPrivileges()) {
        std::cerr << "Unable to add required privileges. Make sure to run this as administrator." << std::endl;
        std::cerr << "Error given: " << getLastErrorMessage() << std::endl;
        return;
    }

    // Run the World of Warcraft executable
    HANDLE wowProcessHandle = NULL;
    if (!createProcess(WOW_EXECUTABLE_PATH, wowProcessHandle)) {
        std::cerr << "Unable to run '" << WOW_EXECUTABLE_PATH << "': " << getLastErrorMessage() << std::endl;
        return;
    }

    std::cout << "WoW process started successfully." << std::endl;

    // Enter the main application loop
    bool gameWasClosed = fovMaintainLoop(wowProcessHandle);
    if (gameWasClosed) {
        std::cout << "The game was closed, exiting successfully." << std::endl;
    }
    closeProcessHandle(wowProcessHandle);
}
