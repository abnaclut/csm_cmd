@echo off
:: ============================================================
:: rebuild_release.bat - Build Release configuration with tests
:: ============================================================
echo.
echo ========================================
echo    Building Release Configuration
echo ========================================
echo.
call "%~dp0rebuild.bat"
if errorlevel 1 pause