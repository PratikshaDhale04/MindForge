@echo off
REM ===========================================================================
REM  QuizMaster - start the game
REM
REM  The MinGW runtime DLLs (libstdc++, freetype, ...) are not in the Windows
REM  system folder, so we add the compiler folder to PATH for this window
REM  before starting QuizMaster.exe.
REM ===========================================================================

setlocal

set "MINGW=C:\msys64\ucrt64"
set "PROJECT=%~dp0"
if "%PROJECT:~-1%"=="\" set "PROJECT=%PROJECT:~0,-1%"
set "EXE=%PROJECT%\build\bin\QuizMaster.exe"

if not exist "%EXE%" (
    echo [ERROR] QuizMaster.exe was not found.
    echo         Build it first by running:  build.bat
    exit /b 1
)

set "PATH=%MINGW%\bin;%PATH%"
pushd "%PROJECT%\build\bin"
"%EXE%"
popd
