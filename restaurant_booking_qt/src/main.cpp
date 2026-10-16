#include "MainWindow.hpp"
#include "DatabaseManager.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QtPlugin>

#ifdef QT_STATIC_BUILD
Q_IMPORT_PLUGIN(QSQLiteDriverPlugin)
#endif

static QString restaurantStyleSheet() {
    return R"(
        QMainWindow, QWidget {
            background-color: #1E202B;
            color: #EAECEE;
            font-family: "Segoe UI", "Noto Sans", sans-serif;
            font-size: 12px;
        }
        QGroupBox {
            border: 1px solid #3A3F54;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
            font-weight: bold;
            color: #F5D76E;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
        }
        QTabWidget::pane {
            border: 1px solid #3A3F54;
            border-radius: 6px;
            background: #242736;
        }
        QTabBar::tab {
            background: #2B2F42;
            color: #BDC3C7;
            padding: 9px 16px;
            margin-right: 4px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-weight: bold;
        }
        QTabBar::tab:selected {
            background: #D4AF37;
            color: #181A24;
        }
        QLineEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit {
            background-color: #161821;
            border: 1px solid #454B66;
            border-radius: 5px;
            padding: 5px 8px;
            color: #FFFFFF;
        }
        QTableWidget, QListWidget {
            background-color: #181A24;
            border: 1px solid #3A3F54;
            border-radius: 6px;
            gridline-color: #2C3042;
            selection-background-color: #3D566E;
        }
        QHeaderView::section {
            background-color: #2B2F42;
            color: #F5D76E;
            padding: 6px;
            border: 1px solid #3A3F54;
            font-weight: bold;
        }
        QPushButton {
            background-color: #343A50;
            color: #FFFFFF;
            border: 1px solid #4E5675;
            border-radius: 6px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #464E6B;
        }
        QProgressBar {
            border: 1px solid #3A3F54;
            border-radius: 5px;
            background-color: #161821;
            text-align: center;
            color: #FFFFFF;
        }
        QProgressBar::chunk {
            background-color: #D4AF37;
            border-radius: 4px;
        }
    )";
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Aura Gastronomy Booking System");
    app.setOrganizationName("UniversityLab");
    app.setStyleSheet(restaurantStyleSheet());

    QString dbPath = QCoreApplication::applicationDirPath() + "/data/restaurant.db";
    if (!QFileInfo::exists(dbPath) && QFileInfo::exists("data/restaurant.db")) {
        dbPath = "data/restaurant.db";
    }

    Restaurant::DatabaseManager db;
    db.openDatabase(dbPath);

    Restaurant::MainWindow window(db);
    window.show();

    return app.exec();
}
