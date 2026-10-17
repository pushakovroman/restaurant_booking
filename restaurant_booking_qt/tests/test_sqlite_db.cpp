// Модульный автотест реляционной базы данных SQLite (data/restaurant.db) для белорусского рынка (BYN, +375)
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
    std::cout << "Запуск тестов базы данных SQLite (Белорусская локализация: BYN и +375)...\n";

    sqlite3* db = nullptr;
    int rc = sqlite3_open("/tmp/restaurant.db", &db);
    assert(rc == 0 && db != nullptr);
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    // 1. Проверка таблиц столов и депозитов в BYN
    assert(querySingleInt(db, "SELECT COUNT(*) FROM tables;") == 12);
    double vipDep = querySingleDouble(db, "SELECT min_deposit FROM tables WHERE code='V1';");
    assert(std::abs(vipDep - 150.0) < 0.01);
    std::cout << " [OK] Тест 1: 12 столов загружены, депозит VIP-ложи V1 = " << vipDep << " BYN.\n";

    // 2. Проверка белорусских телефонных номеров (+375)
    int byPhones = querySingleInt(db, "SELECT COUNT(*) FROM reservations WHERE guest_phone LIKE '+375%';");
    assert(byPhones == 4);
    std::cout << " [OK] Тест 2: Все номера телефонов гостей имеют белорусский код +375.\n";

    // 3. Проверка цен меню в BYN и авторских позиций
    assert(querySingleInt(db, "SELECT COUNT(*) FROM dishes;") == 14);
    double dranikiPrice = querySingleDouble(db, "SELECT price FROM dishes WHERE id=7;");
    assert(std::abs(dranikiPrice - 34.0) < 0.01);
    std::cout << " [OK] Тест 3: Цены меню переведены в BYN (Авторские драники = " << dranikiPrice << " BYN).\n";

    // 4. Проверка расчета предзаказа в BYN
    double preorderSum1001 = querySingleDouble(db,
        "SELECT SUM(d.price * p.quantity) FROM preorder_items p "
        "JOIN dishes d ON p.dish_id = d.id WHERE p.reservation_id = 1001;");
    assert(std::abs(preorderSum1001 - 162.0) < 0.01);
    std::cout << " [OK] Тест 4: Сумма предзаказа брони #1001 составляет " << preorderSum1001 << " BYN.\n";

    sqlite3_close(db);
    std::cout << "\nВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО (4/4)!\n";
    return 0;
}
