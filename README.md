# Path of Exile TCP Disconnect Tool

A lightweight Windows utility that allows you to quickly disconnect from Path of Exile game servers by pressing a mouse button. This tool is useful for situations where you need to immediately disconnect from the game, such as avoiding death in hardcore mode or escaping dangerous encounters.

## Features

- **Configurable Trigger**: Choose from multiple mouse buttons or keyboard keys to trigger the disconnect
- **Quick Disconnect**: Instantly close the game's TCP connection with your configured input
- **Safety Check**: Only works when Path of Exile is the active foreground window, preventing accidental disconnects
- **Multi-Version Support**: Compatible with all Path of Exile versions:
  - `PathOfExile.exe` (Standalone 32-bit)
  - `PathOfExile_x64.exe` (Standalone 64-bit)
  - `PathOfExileSteam.exe` (Steam 32-bit)
  - `PathOfExile_x64Steam.exe` (Steam 64-bit)
- **Minimal Resource Usage**: Runs in the background with minimal CPU and memory footprint
- **No Game Modification**: Works by manipulating TCP connections at the OS level, not by modifying game files

## How It Works

The tool uses Windows API to:
1. Parse the command-line argument to determine which input to use as trigger (defaults to Mouse Button 4)
2. Install a global mouse or keyboard hook to listen for the configured trigger
3. Verify that Path of Exile is the active foreground window when the trigger is activated
4. Find the Path of Exile process ID
5. Locate and close the TCP connection associated with the game process

## Requirements

- **Operating System**: Windows (Windows 7 or later recommended)
- **Compiler**: Microsoft Visual C++ or compatible C++ compiler with Windows SDK
- **Libraries**: 
  - Winsock2 (Ws2_32.lib)
  - IP Helper API (Iphlpapi.lib)
- **Permissions**: Administrator privileges may be required to manipulate TCP connections

## Building

### Using Visual Studio

1. Open Visual Studio Developer Command Prompt
2. Navigate to the project directory
3. Compile with:
```cmd
cl /EHsc /DUNICODE /D_UNICODE poedisconnect.cpp /link Ws2_32.lib Iphlpapi.lib
```

### Using MinGW

```cmd
g++ -DUNICODE -D_UNICODE poedisconnect.cpp -o poedisconnect.exe -lws2_32 -liphlpapi
```

## Usage

1. **Run the application** as Administrator (recommended) with your desired trigger:
   ```cmd
   poedisconnect.exe [trigger]
   ```
   
   Available triggers:
   - **Mouse Buttons**: `mb4` (default), `mb5`, `middle`, `left`, `right`
   - **Keyboard Keys**: `f1`-`f12`, `space`, `insert`, `delete`, `home`, `end`, `pageup`, `pagedown`
   
   Examples:
   ```cmd
   poedisconnect.exe          # Uses MB4 (back button) by default
   poedisconnect.exe mb5      # Use Mouse Button 5 (forward button)
   poedisconnect.exe f9       # Use F9 key
   poedisconnect.exe space    # Use Space bar
   ```

2. **Launch Path of Exile** (any version)

3. **Press your configured trigger** when you need to disconnect

4. The application will:
   - Verify that Path of Exile is in the foreground
   - Close the game's TCP connection
   - Display a confirmation message in the console

5. **To exit** the tool, close the console window or press Ctrl+C

## Console Output

The tool provides feedback in the console window:
- Usage information showing all available triggers
- `"Configured trigger: [trigger name]"` - Shows which input is configured
- `"[Trigger name] Pressed - Closing PoE Connection..."` - Trigger was detected
- `"Closed PoE TCP Connection!"` - Connection successfully closed
- `"PoE is NOT in the foreground. Ignoring disconnect request."` - PoE window not active (safety feature)
- `"PoE process not found!"` - Path of Exile is not running
- `"Failed to install mouse/keyboard hook!"` - Error installing the global hook

## Safety Features

- **Foreground Window Check**: The tool only activates when Path of Exile is the active window, preventing accidental disconnects while you're in other applications
- **Process Verification**: Verifies that the Path of Exile process is running before attempting to close connections

## Limitations

- **Windows Only**: This tool uses Windows-specific APIs and will not work on Linux or macOS
- **Administrator Rights**: May require administrator privileges to manipulate TCP connections

## Troubleshooting

### "Failed to install mouse/keyboard hook!"
- Ensure you're running the application with sufficient privileges
- Check that no other application is interfering with hooks

### "PoE process not found!"
- Verify that Path of Exile is running
- Ensure the process name matches one of the supported versions

### Connection doesn't close
- Run the application as Administrator
- Verify that Path of Exile is the active foreground window
- Check that you're pressing the correct configured trigger
- Ensure your trigger isn't already being used by another application

## Disclaimer

⚠️ **Use at your own risk**. This tool manipulates network connections at the operating system level. While it does not modify game files or memory, using third-party tools with online games may be against the game's Terms of Service. Check the Path of Exile Terms of Service before using this tool.

This tool is provided for educational purposes and demonstrates Windows API usage for process and network management.

## License

This project is provided as-is without any warranty. Use at your own risk.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests.

## Technical Details

### Key Functions

- `GetPoEProcessID()`: Scans running processes to find Path of Exile
- `IsPoEActive()`: Checks if PoE is the active foreground window
- `ClosePoETcpConnection()`: Retrieves TCP table and closes the connection for the PoE process
- `ParseTriggerInput()`: Parses command-line argument to configure the trigger
- `MouseHookProc()`: Callback function for the global mouse hook
- `KeyboardHookProc()`: Callback function for the global keyboard hook
- `SetHook()` / `Unhook()`: Install and remove the appropriate hook (mouse or keyboard)

### Windows APIs Used

- `SetWindowsHookEx()`: Install global mouse hook
- `CreateToolhelp32Snapshot()`: List running processes
- `GetExtendedTcpTable()`: Retrieve active TCP connections
- `SetTcpEntry()`: Modify TCP connection state
- `GetForegroundWindow()`: Check active window
