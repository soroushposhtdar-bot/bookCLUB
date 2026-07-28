@echo off
REM ============================================================
REM BookCLUB — Windows launch script for Qt Creator
REM ============================================================
REM Starts the server, waits 2 seconds, then starts the client.
REM
REM Usage from Qt Creator:
REM   1. Build both targets (BookClubServer + BookClubClient)
REM   2. Add a custom run step that calls this script
REM   3. Or run manually from the build output directory
REM
REM The script expects the executables in .\bin\ (CMake default).
REM ============================================================

setlocal

REM --- Find the build output directory ---
set "BUILD_DIR=%~dp0"
set "BIN_DIR=%BUILD_DIR%bin"

REM --- Start the server in the background ---
echo Starting BookClubServer on port 8080...
start "BookClubServer" "%BIN_DIR%\BookClubServer.exe" -p 8080

REM --- Wait for the server to initialise ---
timeout /t 2 /nobreak >nul

REM --- Start the client ---
echo Starting BookClubClient...
"%BIN_DIR%\BookClubClient.exe"

REM --- When the client exits, kill the server ---
taskkill /fi "WindowTitle eq BookClubServer" >nul 2>&1

endlocal
