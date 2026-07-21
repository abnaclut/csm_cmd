@echo off
:: ============================================================
:: rebuild_no_tests.bat - Build Release without tests
:: ============================================================
echo.
echo ========================================
echo    Building Without Tests
echo ========================================
echo.
call "%~dp0rebuild.bat" --no-tests
if errorlevel 1 pause