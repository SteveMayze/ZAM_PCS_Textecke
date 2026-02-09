@echo off
REM setup.bat - Setup script for Windows
REM
REM This script helps set up the development environment for this PlatformIO project
REM Run this after cloning the repository

echo =========================================
echo TextDisplay Rest Adapter - Setup Script
echo =========================================
echo.

REM Check if we're in the correct directory
if not exist "platformio.ini" (
    echo ERROR: platformio.ini not found!
    echo        Please run this script from the project root directory.
    exit /b 1
)

echo [OK] Found platformio.ini
echo.

REM Check for Python
echo Checking for Python...
python --version >nul 2>&1
if %errorlevel% == 0 (
    python --version
    echo [OK] Python installed
) else (
    echo [ERROR] Python not found!
    echo         Please install Python 3.x from python.org
    echo         Make sure to check "Add Python to PATH" during installation
    exit /b 1
)
echo.

REM Check for PlatformIO
echo Checking for PlatformIO...
pio --version >nul 2>&1
if %errorlevel% == 0 (
    pio --version
    echo [OK] PlatformIO installed
) else (
    echo [WARNING] PlatformIO CLI not found
    echo           Install it with: pip install -U platformio
    echo           Or use the VS Code PlatformIO extension
)
echo.

REM Check for secrets.h
echo Checking for secrets.h...
if exist "src\secrets.h" (
    echo [OK] src\secrets.h exists
) else (
    echo [WARNING] src\secrets.h not found
    if exist "src\secrets.h.template" (
        echo           Creating from template...
        copy src\secrets.h.template src\secrets.h >nul
        echo [OK] Created src\secrets.h from template
        echo.
        echo [IMPORTANT] Edit src\secrets.h with your credentials!
        echo             Update: WiFi SSID, WiFi password, and other secrets
    ) else (
        echo [ERROR] Template src\secrets.h.template not found!
        exit /b 1
    )
)
echo.

REM Next steps
echo =========================================
echo Next Steps:
echo =========================================
echo.
echo 1. Edit src\secrets.h with your credentials (if not done already)
echo 2. Build the project:
echo    pio run -e texteck-esp32-debug
echo.
echo Or use VS Code with PlatformIO extension:
echo    - Open this folder in VS Code
echo    - Click PlatformIO icon in sidebar
echo    - Select environment and click Build
echo.
echo For more details, see PLATFORM_SETUP.md
echo.
echo [OK] Setup complete!
pause
