@echo off
chcp 65001 >nul
title Ender-5 ESP32-CAM GitHub Uploader
echo ========================================================
echo   Ender-5 ESP32-CAM GitHub Yukleme Komekcisi
echo   Repo: https://github.com/emilmahirmahmudov/ender5-esp32cam
echo ========================================================
echo.

cd /d "%~dp0"

echo Fayllar Git ucun hazirlanir...
git add .
git commit -m "feat: complete Ender-5 ESP32-CAM monitoring system with English UI, 3D models and docs" >nul 2>&1

echo.
echo GitHub-a gonderilir...
git push -u origin main

if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================================
    echo   Tebrikler! Layihe ugurla GitHub-a yuklendi!
    echo   Repo: https://github.com/emilmahirmahmudov/ender5-esp32cam
    echo ========================================================
) else (
    echo.
    echo Diqqet: Eger ilk defedirse, brauzerde acilan pencereden GitHub icazesini tesdiqleyin.
)

echo.
pause
