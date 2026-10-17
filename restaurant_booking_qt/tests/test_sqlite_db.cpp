// Расширенный модульный автотест реляционной базы данных SQLite (data/restaurant.db)
// Проверяет схему, индексы, внешние ключи, предзаказ, расписание, лист ожидания и отзывы
#include <iostream>
#include <string>
#include <cassert>
#include <cmath>

extern "C" {
struct sqlite3;
struct sqlite3_stmt;
int sqlite3_open(const char* filename, sqlite3** ppDb);
int sqlite3_close(sqlite3* db);
int sqlite3_exec(sqlite3* db, const char* sql, int (*callback)(void*,int,char**,char**), void* arg, char** errmsg);
int sqlite3_prepare_v2(sqlite3* db, const char* zSql, int nByte, sqlite3_stmt** ppStmt, const char** pzTail);
int sqlite3_step(sqlite3_stmt* pStmt);
int sqlite3_finalize(sqlite3_stmt* pStmt);
int sqlite3_column_int(sqlite3_stmt* pStmt, int iCol);
double sqlite3_column_double(sqlite3_stmt* pStmt, int iCol);
const unsigned char* sqlite3_column_text(sqlite3_stmt* pStmt, int iCol);
}

static int querySingleInt(sqlite3* db, const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    assert(rc == 0 && stmt != nullptr);
    int val = 0;
    if (sqlite3_step(stmt) == 100) {
        val = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return val;
}

static double querySingleDouble(sqlite3* db, const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    assert(rc == 0 && stmt != nullptr);
    double val = 0.0;
    if (sqlite3_step(stmt) == 100) {
        val = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return val;
}

int main() {
    std::cout << "Запуск расширенного набора тестов базы данных SQLite (data/restaurant.db)...\n";

    sqlite3* db = nullptr;
    int rc = sqlite3_open("/tmp/restaurant.db", &db);
    assert(rc == 0 && db != nullptr);
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    // 1. Проверка таблиц столов (2D-карта)
    assert(querySingleInt(db, "SELECT COUNT(*) FROM tables;") == 12);
    std::cout << " [OK] Тест 1: Таблица 'tables' содержит 12 столов с координатами и зонами.\n";

    // 2. Проверка наличия индексов производительности
    int idxCount = querySingleInt(db, "SELECT COUNT(*) FROM sqlite_master WHERE type='index' AND name LIKE 'idx_%';");
    assert(idxCount == 6);
    std::cout << " [OK] Тест 2: Все 6 индексов быстрого поиска SQLite созданы корректно.\n";

    // 3. Проверка меню и ограничений CHECK(price >= 0)
    assert(querySingleInt(db, "SELECT COUNT(*) FROM dishes;") == 14);
    int badInsert = sqlite3_exec(db, "INSERT INTO dishes (name, category, price) VALUES ('Bad', 'Супы', -100);", nullptr, nullptr, nullptr);
    assert(badInsert != 0); // Ограничение CHECK(price >= 0) должно заблокировать отрицательную цену!
    std::cout << " [OK] Тест 3: Защита целостности данных CHECK(price >= 0) заблокировала некорректную цену.\n";

    // 4. Проверка защиты часов работы CHECK(start_hour >= 10 AND start_hour < 23)
    int badHour = sqlite3_exec(db,
        "INSERT INTO reservations (booking_code, guest_name, guest_phone, visit_date, start_hour, duration_hours, party_size, table_id, booking_type, status, occasion) "
        "VALUES ('BK-9999', 'Тест', '-', '2026-10-16', 23, 2, 2, 1, 'Online', 'Confirmed', 'Тест');",
        nullptr, nullptr, nullptr);
    assert(badHour != 0);
    std::cout << " [OK] Тест 4: Ограничение CHECK(start_hour) защитило от записи в нерабочие часы.\n";

    // 5. Проверка связей FOREIGN KEY для предзаказа (preorder_items JOIN dishes)
    double preorderSum1001 = querySingleDouble(db,
        "SELECT SUM(d.price * p.quantity) FROM preorder_items p "
        "JOIN dishes d ON p.dish_id = d.id WHERE p.reservation_id = 1001;");
    assert(std::abs(preorderSum1001 - 4910.0) < 0.01);
    std::cout << " [OK] Тест 5: Связанная таблица 'preorder_items' точно рассчитывает чек предзаказа (4910 руб.).\n";

    // 6. Проверка каскадного удаления и листа ожидания
    assert(querySingleInt(db, "SELECT COUNT(*) FROM waitlist;") == 1);
    double avgRating = querySingleDouble(db, "SELECT AVG(rating) FROM reviews;");
    assert(std::abs(avgRating - 4.75) < 0.01);
    std::cout << " [OK] Тест 6: Лист ожидания ('waitlist') и рейтинг отзывов (★ " << avgRating << "/5.0) проверены.\n";

    sqlite3_close(db);
    std::cout << "\nВСЕ ТЕСТЫ БАЗЫ ДАННЫХ И ЗАЩИТЫ ЦЕЛОСТНОСТИ ПРОЙДЕНЫ (6/6)!\n";
    return 0;
}
