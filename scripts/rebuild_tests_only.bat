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
echo    csm_cmd - BUILD TESTS ONLY
echo ========================================
echo.

set BUILD_DIR=build

if not exist "%BUILD_DIR%" (
    echo [ERROR] Build directory not found.
    echo Run rebuild.bat or rebuild_debug.bat first.
    pause
    exit /b 1
)

where ninja >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Ninja not found in PATH
    pause
    exit /b 1
)

echo [1/3] Entering build directory...
cd "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to enter build directory
    pause
    exit /b 1
)
echo [OK] Entered

echo [2/3] Building tests...
ninja csm_cmd_tests
if errorlevel 1 (
    echo [ERROR] Test build failed
    cd ..
    pause
    exit /b 1
)
echo [OK] Tests built

echo [3/3] Running tests...
ctest --output-on-failure

cd ..

echo.
echo ========================================
echo    TESTS COMPLETE
echo ========================================
echo.

pause