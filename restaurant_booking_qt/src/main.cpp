#include "MainWindow.hpp"
#include "DatabaseManager.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QtPlugin>

#ifdef QT_STATIC_BUILD
Q_IMPORT_PLUGIN(QSQLiteDriverPlugin)
#endif

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Aura Gastronomy Minsk Booking System");
    app.setOrganizationName("UniversityLabBY");
    app.setStyleSheet(Restaurant::MainWindow::darkLuxuryStyleSheet());

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
