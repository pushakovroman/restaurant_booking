cmake_minimum_required(VERSION 3.16)
project(RestaurantBookingQt VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

include_directories(include)

set(PROJECT_SOURCES
    src/main.cpp
    src/DatabaseManager.cpp
    src/FloorMapWidget.cpp
    src/AnalyticsChartWidget.cpp
    src/MainWindow.cpp
    include/Models.hpp
    include/DatabaseManager.hpp
    include/FloorMapWidget.hpp
    include/AnalyticsChartWidget.hpp
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

add_custom_command(TARGET RestaurantBookingApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:RestaurantBookingApp>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/data/restaurant.db"
        "$<TARGET_FILE_DIR:RestaurantBookingApp>/data/restaurant.db"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RestaurantBookingApp)
endif()
