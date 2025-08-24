@echo off
set SCRIPT_DIR=%~dp0
set BAZEL_EXE=%SCRIPT_DIR%bazel.exe
if exist "%BAZEL_EXE%" (
  "%BAZEL_EXE%" %*
  exit /b %ERRORLEVEL%
)

set BAZELISK_PATH=C:\Users\%USERNAME%\AppData\Local\Microsoft\WinGet\Packages\Bazel.Bazelisk_Microsoft.Winget.Source_8wekyb3d8bbwe\bazelisk.exe
if exist "%BAZELISK_PATH%" (
  "%BAZELISK_PATH%" %*
  exit /b %ERRORLEVEL%
)

where bazelisk >nul 2>nul
if %ERRORLEVEL%==0 (
  bazelisk %*
  exit /b %ERRORLEVEL%
)

echo Bazelisk not found. Install Bazelisk or place bazel.exe in repo root.
exit /b 1
