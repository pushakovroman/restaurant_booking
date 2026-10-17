cmake_minimum_required(VERSION 3.16)
project(RestaurantBookingQt VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Опция статической сборки в один автономный .exe файл
option(BUILD_STATIC_EXE "Собрать в единый автономный исполняемый файл (.exe)" OFF)

if(MSVC)
    add_compile_options(/W4 /utf-8)
    if(BUILD_STATIC_EXE)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(BUILD_STATIC_EXE)
        add_link_options(-static -static-libgcc -static-libstdc++)
    endif()
endif()

# Автоматический поиск Qt6 или Qt5 (модули Widgets и Sql для SQLite)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

include_directories(include)

set(PROJECT_SOURCES
    src/main.cpp
    src/DatabaseManager.cpp
    src/FloorMapWidget.cpp
    src/MainWindow.cpp
    include/Models.hpp
    include/DatabaseManager.hpp
    include/SchemaSql.hpp
    include/FloorMapWidget.hpp
    include/MainWindow.hpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(RestaurantBookingApp
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(RestaurantBookingApp ${PROJECT_SOURCES})
endif()

# Включение режима WIN32 (без фонового черного окна консоли в Windows)
set_target_properties(RestaurantBookingApp PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(BUILD_STATIC_EXE)
    target_compile_definitions(RestaurantBookingApp PRIVATE QT_STATIC_BUILD)
endif()

target_link_libraries(RestaurantBookingApp PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

# База данных data/restaurant.db создаётся программой автоматически при первом запуске
# (схема, демо-данные, а для БД старой версии — миграция), поэтому файл БД в проекте не хранится.

# Автотест слоя БД (схема, миграция, триггеры): cmake -DBUILD_TESTS=ON, затем ctest
option(BUILD_TESTS "Собрать автотесты базы данных SQLite" OFF)
if(BUILD_TESTS)
    find_package(SQLite3 REQUIRED)
    enable_testing()
    add_executable(restaurant_sqlite_tests tests/test_sqlite_db.cpp)
    target_include_directories(restaurant_sqlite_tests PRIVATE include)
    target_link_libraries(restaurant_sqlite_tests PRIVATE SQLite::SQLite3)
    add_test(NAME sqlite_db COMMAND restaurant_sqlite_tests)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RestaurantBookingApp)
endif()
