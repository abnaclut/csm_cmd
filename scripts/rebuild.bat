@echo off
setlocal enabledelayedexpansion

:: Переход в корень проекта (где лежит CMakeLists.txt)
cd /d "%~dp0.."
if errorlevel 1 (
    echo [ERROR] Failed to change to project root
    pause
    exit /b 1
)

echo.
echo ========================================
echo    csm_cmd - FORCE REBUILD
echo ========================================
echo.

set BUILD_DIR=build

:: Проверка CMake
where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH
    pause
    exit /b 1
)

:: Проверка Ninja
where ninja >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Ninja not found in PATH
    pause
    exit /b 1
)

:: 1. Удалить build
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

:: 2. Создать build
echo [2/5] Creating build directory...
mkdir "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to create build directory
    pause
    exit /b 1
)
echo [OK] Created

:: 3. Зайти в build
cd "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to enter build directory
    pause
    exit /b 1
)
echo [OK] Entered

:: 4. CMake
echo [3/5] Running CMake...
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DBUILD_TERMINAL_TESTS=ON -DCMAKE_CXX_FLAGS="-mconsole"
if errorlevel 1 (
    echo [ERROR] CMake failed
    cd ..
    pause
    exit /b 1
)
echo [OK] CMake done

:: 5. Сборка ВСЕГО
echo [4/5] Building...
ninja
if errorlevel 1 (
    echo [ERROR] Build failed
    cd ..
    pause
    exit /b 1
)
echo [OK] Build done

:: 6. Тесты
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