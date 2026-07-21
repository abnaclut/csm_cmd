@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0.."
if errorlevel 1 (
    echo [ERROR] Failed to change to project root
    pause
    exit /b 1
)

echo.
echo ========================================
echo    csm_cmd - FORCE REBUILD DEBUG
echo ========================================
echo.

set BUILD_DIR=build_debug

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH
    pause
    exit /b 1
)

where ninja >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Ninja not found in PATH
    pause
    exit /b 1
)

echo [1/5] Deleting build_debug directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    if errorlevel 1 (
        echo [ERROR] Failed to delete build_debug directory
        pause
        exit /b 1
    )
    echo [OK] Deleted
) else (
    echo [OK] Nothing to delete
)

echo [2/5] Creating build_debug directory...
mkdir "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to create build_debug directory
    pause
    exit /b 1
)
echo [OK] Created

cd "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to enter build_debug directory
    pause
    exit /b 1
)
echo [OK] Entered

echo [3/5] Running CMake Debug...
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TERMINAL_TESTS=ON -DCMAKE_CXX_FLAGS="-mconsole"
if errorlevel 1 (
    echo [ERROR] CMake failed
    cd ..
    pause
    exit /b 1
)
echo [OK] CMake done

echo [4/5] Building...
ninja
if errorlevel 1 (
    echo [ERROR] Build failed
    cd ..
    pause
    exit /b 1
)
echo [OK] Build done

echo [5/5] Running tests...
ctest --output-on-failure

cd ..

echo.
echo ========================================
echo    DEBUG BUILD COMPLETE
echo ========================================
echo.

set EXE_PATH=build_debug\bin\csm_terminal.exe
if not exist "%EXE_PATH%" set EXE_PATH=build_debug\csm_terminal.exe

if exist "%EXE_PATH%" (
    echo Executable: %EXE_PATH%
) else (
    echo [WARNING] Executable not found
)

echo.
pause