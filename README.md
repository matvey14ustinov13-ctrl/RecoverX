# RecoverX

Portable Windows utility for system management, recovery, and maintenance. Native C++ / Win32 implementation with a dark interface.

> **Support status:** Program support has ended. The project is provided as is, without further updates or support.
>
> **Статус поддержки:** Поддержка программы завершена. Проект предоставляется как есть, без дальнейших обновлений и поддержки.

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
├── LICENSE
└── README.md
```

Language is stored in `recoverx_config.json` next to the executable.

## License

RecoverX is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

RecoverX is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

The full license text is available in the [LICENSE](LICENSE) file or at <https://www.gnu.org/licenses/gpl-3.0.html>.

The complete source code is available in this repository.
