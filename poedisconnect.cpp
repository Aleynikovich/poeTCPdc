#define UNICODE  // Force Unicode build
#define _UNICODE

#include <winsock2.h>  // Winsock for TCP functions
#include <windows.h>
#include <iphlpapi.h>  // For TCP manipulation
#include <tlhelp32.h>  // For process listing
#include <tchar.h>     // For _tcscmp (cross-compatible string comparison)
#include <iostream>
#include <fstream>
#include <string>
#include <limits>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

HHOOK mouseHook;
HHOOK keyboardHook;

// Configuration structure
struct DisconnectKeyConfig {
    bool isKeyboard;  // true for keyboard, false for mouse
    int keyCode;      // Virtual key code or mouse button code
    bool ctrlPressed;
    bool altPressed;
    bool shiftPressed;
};

DisconnectKeyConfig config = {false, XBUTTON1, false, false, false};  // Default to MB4
const char* CONFIG_FILE = "poedisconnect_config.txt";

// Function to load configuration from file
bool LoadConfig() {
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    if (std::getline(file, line)) {
        config.isKeyboard = (line == "keyboard");
    } else {
        file.close();
        return false;
    }
    
    if (std::getline(file, line)) {
        config.keyCode = std::stoi(line);
    } else {
        file.close();
        return false;
    }
    
    if (std::getline(file, line)) {
        config.ctrlPressed = (line == "1");
    }
    if (std::getline(file, line)) {
        config.altPressed = (line == "1");
    }
    if (std::getline(file, line)) {
        config.shiftPressed = (line == "1");
    }
    
    file.close();
    return true;
}

// Function to save configuration to file
void SaveConfig() {
    std::ofstream file(CONFIG_FILE);
    if (file.is_open()) {
        file << (config.isKeyboard ? "keyboard" : "mouse") << std::endl;
        file << config.keyCode << std::endl;
        file << (config.ctrlPressed ? "1" : "0") << std::endl;
        file << (config.altPressed ? "1" : "0") << std::endl;
        file << (config.shiftPressed ? "1" : "0") << std::endl;
        file.close();
        std::cout << "Configuration saved successfully!\n";
    } else {
        std::cerr << "Failed to save configuration!\n";
    }
}

// Function to get key name from virtual key code
std::string GetKeyName(int vkCode) {
    char keyName[256];
    UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
    if (GetKeyNameTextA(scanCode << 16, keyName, 256)) {
        return std::string(keyName);
    }
    return "Unknown Key";
}

// Function to configure disconnect key on first run
void ConfigureDisconnectKey() {
    std::cout << "\n=== First Time Setup ===\n";
    std::cout << "You haven't configured your disconnect key yet.\n";
    std::cout << "Press any key (with optional Ctrl/Alt/Shift modifiers) and then press ENTER...\n";
    std::cout << "Note: For mouse buttons, just press ENTER to use the default Mouse Button 4.\n\n";
    
    // Flush input buffer
    while (std::cin.peek() != '\n' && std::cin.peek() != EOF) {
        std::cin.get();
    }
    
    // Wait for key press
    bool keyDetected = false;
    int detectedKey = 0;
    bool detectedCtrl = false;
    bool detectedAlt = false;
    bool detectedShift = false;
    
    // Give user time to press a key
    std::cout << "Waiting for key press...\n";
    Sleep(500);  // Small delay to prepare
    
    // Check for modifier keys and any key press
    for (int vk = 0x08; vk <= 0xFE; vk++) {
        // Skip Enter key itself
        if (vk == VK_RETURN) continue;
        
        SHORT keyState = GetAsyncKeyState(vk);
        if (keyState & 0x8000) {  // Key is pressed
            if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL) {
                detectedCtrl = true;
            } else if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU) {
                detectedAlt = true;
            } else if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT) {
                detectedShift = true;
            } else if (!keyDetected) {
                detectedKey = vk;
                keyDetected = true;
            }
        }
    }
    
    // Wait for Enter key
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (keyDetected) {
        config.isKeyboard = true;
        config.keyCode = detectedKey;
        config.ctrlPressed = detectedCtrl;
        config.altPressed = detectedAlt;
        config.shiftPressed = detectedShift;
        
        std::cout << "\nConfigured disconnect key: ";
        if (config.ctrlPressed) std::cout << "Ctrl + ";
        if (config.altPressed) std::cout << "Alt + ";
        if (config.shiftPressed) std::cout << "Shift + ";
        std::cout << GetKeyName(config.keyCode) << std::endl;
    } else {
        // No key detected, use default MB4
        config.isKeyboard = false;
        config.keyCode = XBUTTON1;
        config.ctrlPressed = false;
        config.altPressed = false;
        config.shiftPressed = false;
        std::cout << "\nNo key detected. Using default: Mouse Button 4 (Back Button)\n";
    }
    
    SaveConfig();
    std::cout << "\nSetup complete! The program will now start monitoring...\n\n";
}

// Function to get Path of Exile's process ID
DWORD GetPoEProcessID() {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;  // Use Wide version
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if ((wcscmp(pe.szExeFile, L"PathOfExileSteam.exe")      == 0) || 
                (wcscmp(pe.szExeFile, L"PathOfExile_x64Steam.exe")  == 0) || 
                (wcscmp(pe.szExeFile, L"PathOfExile.exe")           == 0) || 
                (wcscmp(pe.szExeFile, L"PathOfExile_x64.exe")       == 0))
                    {  
                        pid = pe.th32ProcessID;
                        break;
                    }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pid;
}


// Function to check if PoE is the active (foreground) window
bool IsPoEActive() {
    DWORD pid = GetPoEProcessID();
    if (!pid) return false;

    HWND foregroundWindow = GetForegroundWindow();
    DWORD foregroundPid = 0;
    GetWindowThreadProcessId(foregroundWindow, &foregroundPid);

    return (foregroundPid == pid);
}

// Function to close PoE's TCP connection
void ClosePoETcpConnection() {

    if (!IsPoEActive()) {
        std::cout << "PoE is NOT in the foreground. Ignoring disconnect request.\n";
        return;
    }

    DWORD pid = GetPoEProcessID();
    if (!pid) {
        std::cerr << "PoE process not found!\n";
        return;
    }

    // Retrieve TCP table
    PMIB_TCPTABLE_OWNER_PID pTcpTable;
    DWORD size = 0;
    GetExtendedTcpTable(NULL, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    pTcpTable = (PMIB_TCPTABLE_OWNER_PID)malloc(size);
    if (GetExtendedTcpTable(pTcpTable, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            if (pTcpTable->table[i].dwOwningPid == pid) {
                MIB_TCPROW row;
                row.dwState = MIB_TCP_STATE_DELETE_TCB;
                row.dwLocalAddr = pTcpTable->table[i].dwLocalAddr;
                row.dwLocalPort = pTcpTable->table[i].dwLocalPort;
                row.dwRemoteAddr = pTcpTable->table[i].dwRemoteAddr;
                row.dwRemotePort = pTcpTable->table[i].dwRemotePort;
                SetTcpEntry(&row);
                std::cout << "Closed PoE TCP Connection!\n";
                break;
            }
        }
    }
    free(pTcpTable);
}

// Mouse hook callback
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (!config.isKeyboard && nCode >= 0 && wParam == WM_XBUTTONDOWN) {
        MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;
        
        // Check if configured mouse button is pressed
        if (HIWORD(mouseInfo->mouseData) == config.keyCode) {
            std::cout << "Configured Mouse Button Pressed - Closing PoE Connection...\n";
            ClosePoETcpConnection();
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

// Keyboard hook callback
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (config.isKeyboard && nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbInfo = (KBDLLHOOKSTRUCT*)lParam;
        
        // Check if configured key is pressed with correct modifiers
        if (kbInfo->vkCode == config.keyCode) {
            bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            
            if (ctrlDown == config.ctrlPressed && 
                altDown == config.altPressed && 
                shiftDown == config.shiftPressed) {
                std::cout << "Configured Key Pressed - Closing PoE Connection...\n";
                ClosePoETcpConnection();
            }
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Install the mouse hook
void SetHook() {
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
    if (!mouseHook) {
        std::cerr << "Failed to install mouse hook!\n";
        exit(1);
    }
    
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
    if (!keyboardHook) {
        std::cerr << "Failed to install keyboard hook!\n";
        exit(1);
    }
}

// Unhook before exiting
void Unhook() {
    UnhookWindowsHookEx(mouseHook);
    UnhookWindowsHookEx(keyboardHook);
}

// Main loop to keep the program running
int main() {
    std::cout << "=== Path of Exile TCP Disconnect Tool ===\n\n";
    
    // Check if configuration exists
    if (!LoadConfig()) {
        ConfigureDisconnectKey();
    } else {
        std::cout << "Loaded configuration from file.\n";
        std::cout << "Disconnect key: ";
        if (config.isKeyboard) {
            if (config.ctrlPressed) std::cout << "Ctrl + ";
            if (config.altPressed) std::cout << "Alt + ";
            if (config.shiftPressed) std::cout << "Shift + ";
            std::cout << GetKeyName(config.keyCode) << std::endl;
        } else {
            std::cout << "Mouse Button 4 (Back Button)" << std::endl;
        }
        std::cout << std::endl;
    }
    
    std::cout << "Monitoring for disconnect key press...\n";
    std::cout << "Press Ctrl+C to exit.\n\n";
    
    SetHook();
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    Unhook();
    return 0;
}
