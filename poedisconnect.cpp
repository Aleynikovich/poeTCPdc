#define UNICODE  // Force Unicode build
#define _UNICODE

#include <winsock2.h>  // Winsock for TCP functions
#include <windows.h>
#include <iphlpapi.h>  // For TCP manipulation
#include <tlhelp32.h>  // For process listing
#include <tchar.h>     // For _tcscmp (cross-compatible string comparison)
#include <iostream>
#include <string>
#include <map>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

HHOOK mouseHook;
HHOOK keyboardHook;

// Configuration for trigger input
enum InputType { MOUSE_BUTTON, KEYBOARD_KEY };
struct TriggerConfig {
    InputType type;
    int value; // Virtual key code or mouse button identifier
    std::string name;
} triggerConfig;

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
    if (nCode >= 0 && triggerConfig.type == MOUSE_BUTTON) {
        MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;
        bool triggered = false;

        // Check for X buttons (MB4, MB5)
        if (wParam == WM_XBUTTONDOWN) {
            if ((triggerConfig.value == XBUTTON1 && HIWORD(mouseInfo->mouseData) == XBUTTON1) ||
                (triggerConfig.value == XBUTTON2 && HIWORD(mouseInfo->mouseData) == XBUTTON2)) {
                triggered = true;
            }
        }
        // Check for other mouse buttons
        else if ((wParam == WM_LBUTTONDOWN && triggerConfig.value == VK_LBUTTON) ||
                 (wParam == WM_RBUTTONDOWN && triggerConfig.value == VK_RBUTTON) ||
                 (wParam == WM_MBUTTONDOWN && triggerConfig.value == VK_MBUTTON)) {
            triggered = true;
        }

        if (triggered) {
            std::cout << triggerConfig.name << " Pressed - Closing PoE Connection...\n";
            ClosePoETcpConnection();
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

// Keyboard hook callback
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && triggerConfig.type == KEYBOARD_KEY && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* keyInfo = (KBDLLHOOKSTRUCT*)lParam;
        
        if (keyInfo->vkCode == triggerConfig.value) {
            std::cout << triggerConfig.name << " Pressed - Closing PoE Connection...\n";
            ClosePoETcpConnection();
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Install the mouse hook
void SetHook() {
    if (triggerConfig.type == MOUSE_BUTTON) {
        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
        if (!mouseHook) {
            std::cerr << "Failed to install mouse hook!\n";
            exit(1);
        }
    } else {
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
        if (!keyboardHook) {
            std::cerr << "Failed to install keyboard hook!\n";
            exit(1);
        }
    }
}

// Unhook before exiting
void Unhook() {
    if (mouseHook) UnhookWindowsHookEx(mouseHook);
    if (keyboardHook) UnhookWindowsHookEx(keyboardHook);
}

// Parse command line argument to set trigger config
bool ParseTriggerInput(const std::wstring& input) {
    // Map of supported inputs
    static const std::map<std::wstring, TriggerConfig> inputMap = {
        // Mouse buttons
        {L"mb4", {MOUSE_BUTTON, XBUTTON1, "Mouse Button 4"}},
        {L"mb5", {MOUSE_BUTTON, XBUTTON2, "Mouse Button 5"}},
        {L"middle", {MOUSE_BUTTON, VK_MBUTTON, "Middle Mouse Button"}},
        {L"left", {MOUSE_BUTTON, VK_LBUTTON, "Left Mouse Button"}},
        {L"right", {MOUSE_BUTTON, VK_RBUTTON, "Right Mouse Button"}},
        // Function keys
        {L"f1", {KEYBOARD_KEY, VK_F1, "F1"}},
        {L"f2", {KEYBOARD_KEY, VK_F2, "F2"}},
        {L"f3", {KEYBOARD_KEY, VK_F3, "F3"}},
        {L"f4", {KEYBOARD_KEY, VK_F4, "F4"}},
        {L"f5", {KEYBOARD_KEY, VK_F5, "F5"}},
        {L"f6", {KEYBOARD_KEY, VK_F6, "F6"}},
        {L"f7", {KEYBOARD_KEY, VK_F7, "F7"}},
        {L"f8", {KEYBOARD_KEY, VK_F8, "F8"}},
        {L"f9", {KEYBOARD_KEY, VK_F9, "F9"}},
        {L"f10", {KEYBOARD_KEY, VK_F10, "F10"}},
        {L"f11", {KEYBOARD_KEY, VK_F11, "F11"}},
        {L"f12", {KEYBOARD_KEY, VK_F12, "F12"}},
        // Other common keys
        {L"space", {KEYBOARD_KEY, VK_SPACE, "Space"}},
        {L"insert", {KEYBOARD_KEY, VK_INSERT, "Insert"}},
        {L"delete", {KEYBOARD_KEY, VK_DELETE, "Delete"}},
        {L"home", {KEYBOARD_KEY, VK_HOME, "Home"}},
        {L"end", {KEYBOARD_KEY, VK_END, "End"}},
        {L"pageup", {KEYBOARD_KEY, VK_PRIOR, "Page Up"}},
        {L"pagedown", {KEYBOARD_KEY, VK_NEXT, "Page Down"}},
    };

    // Convert input to lowercase for case-insensitive comparison
    std::wstring lowerInput = input;
    for (auto& c : lowerInput) c = towlower(c);

    auto it = inputMap.find(lowerInput);
    if (it != inputMap.end()) {
        triggerConfig = it->second;
        return true;
    }
    return false;
}

// Display usage information
void ShowUsage() {
    std::cout << "\n=== Path of Exile TCP Disconnect Tool ===\n\n";
    std::cout << "Usage: poedisconnect.exe [trigger]\n\n";
    std::cout << "Available triggers:\n";
    std::cout << "  Mouse Buttons:\n";
    std::cout << "    mb4      - Mouse Button 4 (Back button) [DEFAULT]\n";
    std::cout << "    mb5      - Mouse Button 5 (Forward button)\n";
    std::cout << "    middle   - Middle Mouse Button\n";
    std::cout << "    left     - Left Mouse Button\n";
    std::cout << "    right    - Right Mouse Button\n";
    std::cout << "  Keyboard Keys:\n";
    std::cout << "    f1-f12   - Function keys F1 through F12\n";
    std::cout << "    space    - Space bar\n";
    std::cout << "    insert   - Insert key\n";
    std::cout << "    delete   - Delete key\n";
    std::cout << "    home     - Home key\n";
    std::cout << "    end      - End key\n";
    std::cout << "    pageup   - Page Up key\n";
    std::cout << "    pagedown - Page Down key\n";
    std::cout << "\nExamples:\n";
    std::cout << "  poedisconnect.exe mb4\n";
    std::cout << "  poedisconnect.exe f9\n";
    std::cout << "  poedisconnect.exe space\n\n";
}

// Main loop to keep the program running
int main(int argc, char* argv[]) {
    // Set default trigger to MB4 for backward compatibility
    triggerConfig = {MOUSE_BUTTON, XBUTTON1, "Mouse Button 4"};

    // Parse command line arguments
    if (argc > 1) {
        // Convert char* to wstring
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, NULL, 0);
        if (wideLen > 0) {
            std::wstring wideInput(wideLen, 0);
            MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, &wideInput[0], wideLen);
            wideInput.pop_back(); // Remove null terminator
            
            if (!ParseTriggerInput(wideInput)) {
                std::cerr << "Error: Invalid trigger '" << argv[1] << "'\n";
                ShowUsage();
                return 1;
            }
        }
    }

    ShowUsage();
    std::cout << "Configured trigger: " << triggerConfig.name << "\n";
    std::cout << "Listening for input... (Close window or press Ctrl+C to exit)\n\n";

    SetHook();
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    Unhook();
    return 0;
}
