@echo off
setlocal
cd /d "%~dp0"

set "CMAKE="
where cmake >nul 2>nul && set "CMAKE=cmake"
if not defined CMAKE if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not defined CMAKE if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not defined CMAKE if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if not defined CMAKE (
    echo CMake not found. Install Visual Studio C++ tools or CMake.
    pause
    exit /b 1
)

"%CMAKE%" -S . -B build
if errorlevel 1 (
    pause
    exit /b 1
)

"%CMAKE%" --build build --config Release --parallel
if errorlevel 1 (
    pause
    exit /b 1
)

echo.
echo Build complete: build\Release\RecoverX.exe
if /I "%~1"=="run" start "" "build\Release\RecoverX.exe"
exit /b 0
