# RecoverX

**RecoverX** is a powerful portable utility for system management, recovery, and maintenance of Windows. It combines dozens of tools in one compact window with a dark interface.

## Features

### Power
- Shutdown/restart timer with countdown and sound alert.
- Instant shutdown, restart, sleep.

### Boot
- Reboot to UEFI/BIOS.
- Safe Mode (minimal, with networking), reset Safe Mode.
- Built-in Boot Menu simulator.

### System
- Quick access to system utilities: msinfo32, Device Manager, Task Manager, Disk Management, Local Users and Groups, Registry Editor, Control Panel, On-Screen Keyboard, Magnifier.
- Minimize all windows, reset GPU driver, AppsFolder, PowerShell, change system time.
- Flush DNS cache, Services Manager, backup registry.

### Maintenance
- Clean temporary files (Temp, Prefetch).
- System File Check (SFC).
- System Image Repair (DISM).
- Remove Windows update leftovers.
- Run Malicious Software Removal Tool (MRT).

### Scheduler
- Create shutdown/restart tasks by day of week and time.

### Monitoring
- Display CPU and RAM usage (requires `psutil`, optional).

### Restore Points
- View, create, delete restore points.
- Quick launch of System Restore.

### Windows Update & Defender
- Check for Windows updates.
- Disable automatic updates.
- Enable/disable Windows Defender (requires restart).

### UWP Apps
- List and completely remove UWP apps (including for all users).

### Drivers
- Links to official driver download pages for NVIDIA, AMD, Intel.

### Settings
- Minimize to system tray (icon near clock).
- Autostart with Windows.
- Switch language (English / Russian).
- Check for RecoverX updates (via GitHub).

## Download & Setup

1. Download the latest version from the [Releases](https://github.com/BackupTeam/RecoverX/releases) page.
2. Extract the archive to any folder.
3. Run `RecoverX.exe`.

> **Note:** some features require administrator rights. The program will prompt you to restart as administrator when necessary.

## Requirements

- Windows 10 / 11 (64-bit)
- For resource monitoring, install `psutil` (optional):
  ```bash
  pip install psutil