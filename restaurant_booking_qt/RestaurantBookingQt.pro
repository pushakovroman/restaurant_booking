QT       += core gui widgets sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = RestaurantBookingApp
TEMPLATE = app

INCLUDEPATH += include

HEADERS += \
    include/Models.hpp \
    include/DatabaseManager.hpp \
    include/FloorMapWidget.hpp \
    include/AnalyticsChartWidget.hpp \
    include/MainWindow.hpp

SOURCES += \
    src/main.cpp \
    src/DatabaseManager.cpp \
    src/FloorMapWidget.cpp \
    src/AnalyticsChartWidget.cpp \
    src/MainWindow.cpp
