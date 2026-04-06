@echo off
setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0\..\..
for %%I in ("%ROOT_DIR%") do set ROOT_DIR=%%~fI
set BUILD_DIR=%ROOT_DIR%\build-windows
set EXE_PATH=%BUILD_DIR%\Release\Jingle Palette Pro.exe
set ISS_FILE=%ROOT_DIR%\JinglePalettePro.iss

echo [INFO] Building Release executable...
cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :error
cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 goto :error

if not exist "%EXE_PATH%" (
  echo [ERROR] Executable not found: %EXE_PATH%
  exit /b 1
)

if not exist "%ISS_FILE%" (
  echo [ERROR] Inno Setup script not found: %ISS_FILE%
  exit /b 1
)

echo [INFO] Deploying Qt runtime with windeployqt...
where windeployqt >nul 2>nul
if errorlevel 1 (
  echo [ERROR] windeployqt not found in PATH.
  echo         Open Qt command prompt or add Qt bin folder to PATH.
  exit /b 1
)
windeployqt --release --compiler-runtime "%EXE_PATH%"
if errorlevel 1 goto :error

echo [INFO] Building installer with Inno Setup...
where iscc >nul 2>nul
if errorlevel 1 (
  echo [ERROR] iscc not found in PATH.
  echo         Install Inno Setup and add ISCC to PATH.
  exit /b 1
)
iscc "%ISS_FILE%"
if errorlevel 1 goto :error

echo [OK] Installer created in %ROOT_DIR%\build\installer\
goto :eof

:error
echo [ERROR] Packaging failed.
exit /b 1
