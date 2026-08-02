@echo off
setlocal enabledelayedexpansion

REM ============================================================
REM  STC12 door lock - build script (CMake + Ninja + SDCC)
REM
REM  Usage:
REM    build.bat          configure (if needed) and build
REM    build.bat clean    remove the build directory
REM    build.bat rebuild  clean, then configure and build
REM ============================================================

set "BUILD_DIR=build"
set "GENERATOR=Ninja"
set "SDCC_BIN=C:\Program Files\SDCC\bin"
set "LOG_FILE=%TEMP%\stc12_build_log.txt"

REM --- SDCC is not on the system PATH; add it for this session ---
if exist "%SDCC_BIN%" (
    set "PATH=%SDCC_BIN%;%PATH%"
) else (
    echo [build.bat] ERROR: SDCC not found at "%SDCC_BIN%".
    echo [build.bat] Install SDCC or edit SDCC_BIN in this script.
    exit /b 1
)

REM --- tool checks ---
where sdcc  >nul 2>&1 || ( echo [build.bat] ERROR: sdcc not on PATH.  & exit /b 1 )
where cmake >nul 2>&1 || ( echo [build.bat] ERROR: cmake not on PATH. & exit /b 1 )
where ninja >nul 2>&1 || ( echo [build.bat] ERROR: ninja not on PATH. & exit /b 1 )

REM --- clean / rebuild handling ---
if /i "%~1"=="clean" (
    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
    echo [build.bat] Cleaned %BUILD_DIR%.
    exit /b 0
)
if /i "%~1"=="rebuild" (
    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
)

REM --- configure if the build dir is missing ---
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [build.bat] Configuring...
    cmake -G "%GENERATOR%" -B %BUILD_DIR% -S .
    if %ERRORLEVEL% neq 0 goto :fail
)

REM --- incremental build ---
cmake --build %BUILD_DIR% > "%LOG_FILE%" 2>&1
set BUILD_RET=%ERRORLEVEL%
type "%LOG_FILE%"
if %BUILD_RET% equ 0 goto :done

REM --- recover from a stale cache (e.g. generator changed) ---
findstr /i "Does not match the generator" "%LOG_FILE%" >nul 2>&1
if %ERRORLEVEL% neq 0 (
    findstr /i "CMakeCache.txt" "%LOG_FILE%" >nul 2>&1
    if %ERRORLEVEL% neq 0 goto :fail
)

echo [build.bat] Stale cache detected, cleaning and reconfiguring...
if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
cmake -G "%GENERATOR%" -B %BUILD_DIR% -S .
if %ERRORLEVEL% neq 0 goto :fail
cmake --build %BUILD_DIR%
if %ERRORLEVEL% neq 0 goto :fail

:done
del "%LOG_FILE%" >nul 2>&1
echo [build.bat] OK -^> output\SimpleDoorLock.hex
exit /b 0

:fail
del "%LOG_FILE%" >nul 2>&1
echo [build.bat] Build failed.
exit /b 1
