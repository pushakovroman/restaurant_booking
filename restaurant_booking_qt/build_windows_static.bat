@echo off
chcp 65001 > nul
echo ============================================================================
echo   Сборка проекта RestaurantBookingQt (C++17 / Qt / SQLite) для Windows
echo ============================================================================

mkdir build_release 2>nul
cd build_release

echo [1/2] Конфигурация CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_EXE=ON
if %ERRORLEVEL% NEQ 0 (
    echo [!] Статический профиль Qt не обнаружен, переключаемся на стандартную Release-сборку...
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_EXE=OFF
)

echo [2/2] Компиляция исполняемого файла RestaurantBookingApp.exe...
cmake --build . --config Release

echo.
echo Готово! Исполняемый файл и папка data\restaurant.db находятся в папке build_release.
pause
