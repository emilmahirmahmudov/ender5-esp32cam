@echo off
chcp 65001 >nul
title Ender-5 ESP32-CAM GitHub Uploader
echo ========================================================
echo   Ender-5 ESP32-CAM GitHub Yukleme Komekcisi
echo   Repo: https://github.com/emilmahirmahmudov/ender5-esp32cam
echo ========================================================
echo.

cd /d "%~dp0"

echo 3D Mount modeli kopyalanir...
if exist "d:\Ender5_Camera_mount.stl" (
    copy /y "d:\Ender5_Camera_mount.stl" "3d_models\ender5_camera_mount.stl" >nul
    echo [OK] d:\Ender5_Camera_mount.stl -> 3d_models\ender5_camera_mount.stl kopyalandi!
)

echo.
echo Fayllar Git ucun hazirlanir...
git add .
git commit -m "feat: add 3D camera mount model, license, docs, and English firmware" >nul 2>&1

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
