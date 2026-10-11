@echo off
REM ===========================================================================
REM  QuizMaster - build helper for Windows
REM
REM    build.bat          configure + build the project
REM    build.bat clean    delete the build folder first, then build
REM
REM  It looks for CMake and for an MSYS2 / MinGW toolchain automatically.
REM  If your setup is different, open this file and edit the two paths below.
REM ===========================================================================

setlocal

REM ---- Edit these two lines if the tools are somewhere else -----------------
set "MINGW=C:\msys64\ucrt64"
set "CMAKE=C:\Users\prati\AppData\Local\Programs\Python\Python311\Scripts\cmake.exe"
REM --------------------------------------------------------------------------

set "PROJECT=%~dp0"
if "%PROJECT:~-1%"=="\" set "PROJECT=%PROJECT:~0,-1%"

if /i "%~1"=="clean" (
    echo Removing the build folder...
    rmdir /s /q "%PROJECT%\build" 2>nul
)

if not exist "%MINGW%\bin\g++.exe" (
    echo [ERROR] No C++ compiler found at "%MINGW%\bin\g++.exe".
    echo         Install MSYS2 and run:  pacman -S mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-sfml
    echo         Then edit MINGW in this file.
    exit /b 1
)

if not exist "%CMAKE%" (
    echo [ERROR] CMake not found at "%CMAKE%".
    echo         Install it with:  pip install cmake
    echo         Then edit CMAKE in this file.
    exit /b 1
)

REM The compiler and its runtime DLLs must be on PATH.
set "PATH=%MINGW%\bin;%PATH%"

echo.
echo [1/2] Configuring...
"%CMAKE%" -S "%PROJECT%" -B "%PROJECT%\build" -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_MAKE_PROGRAM="%MINGW%\bin\ninja.exe" ^
    -DCMAKE_CXX_COMPILER="%MINGW%\bin\g++.exe" ^
    -DQUIZMASTER_DEP_PREFIX="%MINGW:\=/%"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo.
echo [2/2] Building...
"%CMAKE%" --build "%PROJECT%\build"
if errorlevel 1 (
    echo [ERROR] The build failed. The compiler messages are above.
    exit /b 1
)

echo.
echo [OK] QuizMaster.exe is in:  %PROJECT%\build\bin
echo      Start it with:  run_quizmaster.bat
exit /b 0
