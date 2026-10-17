QT       += core gui widgets sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Для статической сборки в один .exe файл раскомментируйте следующие две строки:
# CONFIG += static
# QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++

TARGET = RestaurantBookingApp
TEMPLATE = app

INCLUDEPATH += include

HEADERS += \
    include/Models.hpp \
    include/DatabaseManager.hpp \
    include/FloorMapWidget.hpp \
    include/MainWindow.hpp

SOURCES += \
    src/main.cpp \
    src/DatabaseManager.cpp \
    src/FloorMapWidget.cpp \
    src/MainWindow.cpp
