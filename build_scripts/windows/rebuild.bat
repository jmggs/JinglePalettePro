@echo off
setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0\..\..
for %%I in ("%ROOT_DIR%") do set ROOT_DIR=%%~fI
set BUILD_DIR=%ROOT_DIR%\build-windows

echo [INFO] Removing %BUILD_DIR%...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"

echo [INFO] Configuring CMake project...
cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :error

echo [INFO] Building...
cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 goto :error

echo [OK] Clean build finished: %BUILD_DIR%\Release\Jingle Palette Pro.exe
goto :eof

:error
echo [ERROR] Rebuild failed.
exit /b 1
