# RecoverX

Portable Windows utility for system management, recovery, and maintenance. Native C++ / Win32 implementation with a dark interface.

## Features

- **Power** — shutdown/restart timer with countdown and sound alert, sleep, instant shutdown, forced shutdown.
- **Boot** — reboot to UEFI/BIOS, Safe Mode (minimal, with networking), reset Safe Mode, Boot Menu simulator.
- **System** — quick access to system utilities, Device Manager, Task Manager, Disk Management, Local Users, Registry Editor, Control Panel, On-Screen Keyboard, Magnifier.
- **System actions** — minimize all windows, reset GPU driver, AppsFolder, PowerShell, system time, flush DNS, Services, registry backup.
- **Maintenance** — clean temporary files, SFC scan, DISM repair, remove update leftovers, run MRT.
- **Scheduler** — create weekly shutdown/restart tasks by day of week and time.
- **Monitor** — real-time CPU and RAM usage.
- **Restore Points** — list, create, delete restore points and open System Restore.
- **Windows Update** — open update settings, disable automatic updates, manage Windows Defender policy.
- **UWP Apps** — list and remove installed packages.
- **Drivers** — quick links to NVIDIA, AMD, and Intel driver pages.
- **Settings** — tray behavior, autostart, startup settings, update check, English/Russian language switch.

## Requirements

- Windows 10 or Windows 11 (x64)
- Visual Studio 2022/2026 with C++ desktop tools
- CMake 3.20 or newer

## Build

```bat
build.bat
```

Or manually:

```bat
cmake -S . -B build
cmake --build build --config Release
```

The executable is generated at `build\Release\RecoverX.exe`.

## Run

```bat
build\Release\RecoverX.exe
```

Use `build.bat run` to build and launch in one step. Administrator rights are requested for actions that require them.

## Project structure

```text
RecoverX/
├── src/main.cpp
├── res/RecoverX.rc
├── res/recoverx.ico
├── res/app.manifest
├── CMakeLists.txt
├── build.bat
└── README.md
```

Language is stored in `recoverx_config.json` next to the executable.
