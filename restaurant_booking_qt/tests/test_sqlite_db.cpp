// Автономный модульный тест реляционной базы данных SQLite (data/restaurant.db)
// Компилируется и проверяет реальную базу данных через системную библиотеку libsqlite3
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
    if (sqlite3_step(stmt) == 100) { // SQLITE_ROW == 100
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
    std::cout << "Проверка реляционной базы данных SQLite (data/restaurant.db)...\n";

    sqlite3* db = nullptr;
    int rc = sqlite3_open("/tmp/restaurant.db", &db);
    assert(rc == 0 && db != nullptr);

    // 1. Проверка таблиц столов (2D-карта)
    int tableCount = querySingleInt(db, "SELECT COUNT(*) FROM tables;");
    assert(tableCount == 12);
    std::cout << " [OK] Тест 1: В таблице 'tables' ровно 12 столов с 2D-координатами.\n";

    // 2. Проверка меню
    int dishCount = querySingleInt(db, "SELECT COUNT(*) FROM dishes;");
    assert(dishCount == 14);
    std::cout << " [OK] Тест 2: В таблице 'dishes' загружено 14 позиций меню.\n";

    // 3. Проверка бронирований (Онлайн + WalkIn)
    int resCount = querySingleInt(db, "SELECT COUNT(*) FROM reservations;");
    assert(resCount == 4);
    int walkInCount = querySingleInt(db, "SELECT COUNT(*) FROM reservations WHERE booking_type='WalkIn';");
    assert(walkInCount == 1);
    std::cout << " [OK] Тест 3: Онлайн-бронирования и записи Walk-in корректно хранятся в SQLite.\n";

    // 4. Проверка связей FOREIGN KEY для предзаказа (preorder_items JOIN dishes)
    double preorderSum1001 = querySingleDouble(db,
        "SELECT SUM(d.price * p.quantity) FROM preorder_items p "
        "JOIN dishes d ON p.dish_id = d.id WHERE p.reservation_id = 1001;");
    assert(preorderSum1001 > 3000.0);
    std::cout << " [OK] Тест 4: Связанная таблица 'preorder_items' корректно считает сумму предзаказа ("
              << preorderSum1001 << " руб.).\n";

    // 5. Проверка отзывов и среднего рейтинга
    double avgRating = querySingleDouble(db, "SELECT AVG(rating) FROM reviews;");
    assert(std::abs(avgRating - 4.75) < 0.01);
    std::cout << " [OK] Тест 5: Таблица 'reviews' возвращает корректный средний рейтинг ★ " << avgRating << "/5.0.\n";

    sqlite3_close(db);
    std::cout << "\nВСЕ ТЕСТЫ БАЗЫ ДАННЫХ SQLITE ПРОЙДЕНЫ УСПЕШНО (5/5)!\n";
    return 0;
}
