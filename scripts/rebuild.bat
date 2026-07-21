@echo off
setlocal enabledelayedexpansion

:: goto proj root
cd /d "%~dp0.."
if errorlevel 1 (
    echo [ERROR] Failed to change to project root
    pause
    exit /b 1
)

echo.
echo ========================================
echo    csm_cmd - DEFAULT REBUILD
echo ========================================
echo.

set BUILD_DIR=build

:: check CMake
where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH
    pause
    exit /b 1
)

:: check Ninja
where ninja >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Ninja not found in PATH
    pause
    exit /b 1
)

:: delete old build dir
echo [1/5] Deleting build directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    if errorlevel 1 (
        echo [ERROR] Failed to delete build directory
        pause
        exit /b 1
    )
    echo [OK] Deleted
) else (
    echo [OK] Nothing to delete
)

:: create new build dir
echo [2/5] Creating build directory...
mkdir "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to create build directory
    pause
    exit /b 1
)
echo [OK] Created

:: cd build
cd "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to enter build directory
    pause
    exit /b 1
)
echo [OK] Entered

:: CMake
echo [3/5] Running CMake...
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DBUILD_TERMINAL_TESTS=ON -DCMAKE_CXX_FLAGS="-mconsole"
if errorlevel 1 (
    echo [ERROR] CMake failed
    cd ..
    pause
    exit /b 1
)
echo [OK] CMake done

:: build everything
echo [4/5] Building...
ninja
if errorlevel 1 (
    echo [ERROR] Build failed
    cd ..
    pause
    exit /b 1
)
echo [OK] Build done

:: run tests
echo [5/5] Running tests...
ctest --output-on-failure

cd ..

echo.
echo ========================================
echo    BUILD COMPLETE
echo ========================================
echo.

set EXE_PATH=build\bin\csm_terminal.exe
if not exist "%EXE_PATH%" set EXE_PATH=build\csm_terminal.exe

if exist "%EXE_PATH%" (
    echo Executable: %EXE_PATH%
) else (
    echo [WARNING] Executable not found
)

echo.
pause