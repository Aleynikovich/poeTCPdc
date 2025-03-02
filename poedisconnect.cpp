#define UNICODE  // Force Unicode build
#define _UNICODE

#include <winsock2.h>  // Winsock for TCP functions
#include <windows.h>
#include <iphlpapi.h>  // For TCP manipulation
#include <tlhelp32.h>  // For process listing
#include <tchar.h>     // For _tcscmp (cross-compatible string comparison)
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

HHOOK mouseHook;

// Function to get Path of Exile's process ID
DWORD GetPoEProcessID() {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;  // Use Wide version
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, L"PathOfExileSteam.exe") == 0) {  
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pid;
}

// Function to close PoE's TCP connection
void ClosePoETcpConnection() {
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
    if (nCode >= 0 && wParam == WM_XBUTTONDOWN) {
        MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;

        if (HIWORD(mouseInfo->mouseData) == XBUTTON1) { // MB4 (Back Button)
            std::cout << "Mouse Button 4 Pressed - Closing PoE Connection...\n";
            ClosePoETcpConnection();
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

// Install the mouse hook
void SetHook() {
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
    if (!mouseHook) {
        std::cerr << "Failed to install mouse hook!\n";
        exit(1);
    }
}

// Unhook before exiting
void Unhook() {
    UnhookWindowsHookEx(mouseHook);
}

// Main loop to keep the program running
int main() {
    SetHook();
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    Unhook();
    return 0;
}
