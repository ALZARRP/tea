@echo off
echo Installing required Arduino libraries for the VENDOR.ME Cheat Console...

REM Make sure arduino-cli is in the PATH
where arduino-cli >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: arduino-cli not found in your PATH.
    echo Please install it from https://arduino.github.io/arduino-cli/latest/
    exit /b 1
)

echo Updating core index...
arduino-cli core update-index

echo Installing ESP32 core if not present...
arduino-cli core install esp32:esp32

echo Installing libraries...
arduino-cli lib install "ESPAsyncWebServer"
arduino-cli lib install "AsyncTCP"
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "TFT_eSPI"
arduino-cli lib install "AsyncElegantOTA"

echo.
echo All libraries installed successfully.
echo You may need to configure TFT_eSPI for your specific display by editing its User_Setup.h file.
pause
