# Path of Exile TCP Disconnect Tool

A lightweight Windows utility that allows you to quickly disconnect from Path of Exile game servers by pressing a mouse button. This tool is useful for situations where you need to immediately disconnect from the game, such as avoiding death in hardcore mode or escaping dangerous encounters.

## Features

- **Configurable Disconnect Key**: On first run, choose your own keyboard key or key combination (e.g., F9, Ctrl+D) or stick with the default Mouse Button 4
- **Quick Disconnect**: Press your configured key to instantly close the game's TCP connection
- **Safety Check**: Only works when Path of Exile is the active foreground window, preventing accidental disconnects
- **Multi-Version Support**: Compatible with all Path of Exile versions:
  - `PathOfExile.exe` (Standalone 32-bit)
  - `PathOfExile_x64.exe` (Standalone 64-bit)
  - `PathOfExileSteam.exe` (Steam 32-bit)
  - `PathOfExile_x64Steam.exe` (Steam 64-bit)
- **Minimal Resource Usage**: Runs in the background with minimal CPU and memory footprint
- **No Game Modification**: Works by manipulating TCP connections at the OS level, not by modifying game files
- **Persistent Configuration**: Your key preference is saved and remembered for future runs

## How It Works

The tool uses Windows API to:
1. On first run, prompt you to configure your disconnect key (any keyboard key with optional Ctrl/Alt/Shift modifiers, or default to Mouse Button 4)
2. Save your key preference to a configuration file (`poedisconnect_config.txt`)
3. Install global keyboard and mouse hooks to listen for your configured key
4. Verify that Path of Exile is the active foreground window when the key is pressed
5. Find the Path of Exile process ID
6. Locate and close the TCP connection associated with the game process

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
g++ -municode -DUNICODE -D_UNICODE poedisconnect.cpp -o poedisconnect.exe -lws2_32 -liphlpapi
```

## Usage

1. **Run the application** as Administrator (recommended):
   ```cmd
   poedisconnect.exe
   ```

2. **First Time Setup** (only on first run):
   - The program will prompt you to configure your disconnect key
   - Press any key (with optional Ctrl/Alt/Shift modifiers) and then press ENTER
   - Or just press ENTER to use the default Mouse Button 4 (back button)
   - Your choice will be saved to `poedisconnect_config.txt`

3. **Launch Path of Exile** (any version)

4. **Press your configured key** when you need to disconnect

5. The application will:
   - Verify that Path of Exile is in the foreground
   - Close the game's TCP connection
   - Display a confirmation message in the console

6. **To exit** the tool, close the console window or press Ctrl+C

7. **To reconfigure** your disconnect key, simply delete `poedisconnect_config.txt` and restart the application

## Console Output

The tool provides feedback in the console window:
- `"=== First Time Setup ==="` - Configuration wizard on first run
- `"Configured disconnect key: [key]"` - Shows your configured key combination
- `"Configured Key Pressed - Closing PoE Connection..."` - Your key was detected
- `"Closed PoE TCP Connection!"` - Connection successfully closed
- `"PoE is NOT in the foreground. Ignoring disconnect request."` - PoE window not active (safety feature)
- `"PoE process not found!"` - Path of Exile is not running
- `"Failed to install mouse hook!"` or `"Failed to install keyboard hook!"` - Error installing hooks

## Safety Features

- **Foreground Window Check**: The tool only activates when Path of Exile is the active window, preventing accidental disconnects while you're in other applications
- **Process Verification**: Verifies that the Path of Exile process is running before attempting to close connections

## Limitations

- **Windows Only**: This tool uses Windows-specific APIs and will not work on Linux or macOS
- **Administrator Rights**: May require administrator privileges to manipulate TCP connections
- **Single Key Configuration**: Only one key or key combination can be configured at a time (delete config file to reconfigure)

## Troubleshooting

### "Failed to install mouse hook!"
- Ensure you're running the application with sufficient privileges
- Check that no other application is interfering with mouse hooks

### "PoE process not found!"
- Verify that Path of Exile is running
- Ensure the process name matches one of the supported versions

### Connection doesn't close
- Run the application as Administrator
- Verify that Path of Exile is the active foreground window
- Check that you're pressing the correct key/button you configured
- Try deleting `poedisconnect_config.txt` and reconfiguring your disconnect key

## Disclaimer

⚠️ **Use at your own risk**. This tool manipulates network connections at the operating system level. While it does not modify game files or memory, using third-party tools with online games may be against the game's Terms of Service. Check the Path of Exile Terms of Service before using this tool.

This tool is provided for educational purposes and demonstrates Windows API usage for process and network management.

## License

This project is provided as-is without any warranty. Use at your own risk.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests.

## Technical Details

### Key Functions

- `LoadConfig()` / `SaveConfig()`: Load and save disconnect key configuration from/to file
- `ConfigureDisconnectKey()`: First-run setup wizard for configuring the disconnect key
- `GetPoEProcessID()`: Scans running processes to find Path of Exile
- `IsPoEActive()`: Checks if PoE is the active foreground window
- `ClosePoETcpConnection()`: Retrieves TCP table and closes the connection for the PoE process
- `MouseHookProc()`: Callback function for the global mouse hook
- `KeyboardHookProc()`: Callback function for the global keyboard hook
- `SetHook()` / `Unhook()`: Install and remove the mouse and keyboard hooks

### Windows APIs Used

- `SetWindowsHookEx()`: Install global mouse hook
- `CreateToolhelp32Snapshot()`: List running processes
- `GetExtendedTcpTable()`: Retrieve active TCP connections
- `SetTcpEntry()`: Modify TCP connection state
- `GetForegroundWindow()`: Check active window
