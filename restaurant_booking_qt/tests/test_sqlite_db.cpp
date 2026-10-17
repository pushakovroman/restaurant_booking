// Автономный тест слоя базы данных (без Qt): схема, миграция старой БД и триггеры целостности.
// Использует тот же SQL, что и приложение (include/SchemaSql.hpp).
//
// Сборка:  g++ -std=c++17 -Iinclude tests/test_sqlite_db.cpp -lsqlite3 -o restaurant_sqlite_tests
// Запуск:  ./restaurant_sqlite_tests [путь/к/restaurant.db]
//   Если указан путь к существующей БД (например, старой версии), тест проверит её миграцию на КОПИИ файла.
#include "SchemaSql.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

extern "C" {
struct sqlite3;
struct sqlite3_stmt;
int sqlite3_open(const char* filename, sqlite3** ppDb);
int sqlite3_close(sqlite3* db);
int sqlite3_exec(sqlite3* db, const char* sql, int (*callback)(void*, int, char**, char**), void* arg, char** errmsg);
const char* sqlite3_errmsg(sqlite3* db);
int sqlite3_prepare_v2(sqlite3* db, const char* zSql, int nByte, sqlite3_stmt** ppStmt, const char** pzTail);
int sqlite3_step(sqlite3_stmt* pStmt);
int sqlite3_finalize(sqlite3_stmt* pStmt);
int sqlite3_column_int(sqlite3_stmt* pStmt, int iCol);
const unsigned char* sqlite3_column_text(sqlite3_stmt* pStmt, int iCol);
}

namespace {

int g_checks = 0;
int g_failed = 0;

void check(bool condition, const std::string& title) {
    ++g_checks;
    if (condition) {
        std::cout << " [OK]   " << title << "\n";
    } else {
        ++g_failed;
        std::cout << " [FAIL] " << title << "\n";
    }
}

// Выполняет SQL. Возвращает true при успехе, иначе в err — текст ошибки
bool run(sqlite3* db, const std::string& sql, std::string* err = nullptr) {
    char* msg = nullptr;
    const int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &msg);
    if (rc != 0 && err) *err = sqlite3_errmsg(db);
    return rc == 0;
}

bool failsWith(sqlite3* db, const std::string& sql, const std::string& expectedText) {
    std::string err;
    if (run(db, sql, &err)) return false;
    return err.find(expectedText) != std::string::npos;
}

int queryInt(sqlite3* db, const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int value = -1;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == 0) {
        if (sqlite3_step(stmt) == 100) value = sqlite3_column_int(stmt, 0);
    }
    if (stmt) sqlite3_finalize(stmt);
    return value;
}

std::string queryText(sqlite3* db, const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    std::string value;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == 0) {
        if (sqlite3_step(stmt) == 100) {
            const unsigned char* t = sqlite3_column_text(stmt, 0);
            if (t) value = reinterpret_cast<const char*>(t);
        }
    }
    if (stmt) sqlite3_finalize(stmt);
    return value;
}

bool columnExists(sqlite3* db, const std::string& table, const std::string& column) {
    return queryInt(db, "SELECT COUNT(*) FROM pragma_table_info('" + table + "') WHERE name = '" + column + "'") > 0;
}

// Те же шаги, что делает DatabaseManager::initializeSchemaAndSeedData()
bool applySchema(sqlite3* db) {
    if (!run(db, "PRAGMA foreign_keys = ON")) return false;
    for (const char* sql : Restaurant::Schema::kTableStatements) {
        if (!run(db, sql)) return false;
    }
    for (const auto& m : Restaurant::Schema::kColumnMigrations) {
        if (!columnExists(db, m.table, m.column) && !run(db, m.ddl)) return false;
    }
    for (const char* sql : Restaurant::Schema::kIndexAndTriggerStatements) {
        if (!run(db, sql)) return false;
    }
    return true;
}

void insertBaseData(sqlite3* db) {
    run(db, "INSERT INTO tables VALUES (1,'W1','Окна',2,0,0,1,1,0,1,0.0,'d')");
    run(db, "INSERT INTO tables (id,code,zone,capacity,grid_x,grid_y,has_outlet,is_quiet,has_sofa,near_window,min_deposit,description) "
            "VALUES (2,'M1','Зал',4,1,0,0,0,0,0,0.0,'d')");
    run(db, "INSERT INTO dishes (id,name,category,price,weight_grams,calories,prep_time_min,tags,description) "
            "VALUES (1,'Тест','Закуски',100,100,100,5,'','d')");
}

std::string reservation(int id, int tableId, int start, int dur, int party, const std::string& status) {
    return "INSERT INTO reservations (id,booking_code,guest_name,guest_phone,visit_date,start_hour,duration_hours,"
           "party_size,table_id,booking_type,status,occasion,comment) VALUES (" +
           std::to_string(id) + ",'BK-" + std::to_string(id) + "','Гость','+375291112233','2030-01-01'," +
           std::to_string(start) + "," + std::to_string(dur) + "," + std::to_string(party) + "," +
           std::to_string(tableId) + ",'Online','" + status + "','Обычный визит','')";
}

void testFreshDatabase() {
    std::cout << "\n== Новая БД: схема и триггеры ==\n";
    sqlite3* db = nullptr;
    sqlite3_open(":memory:", &db);
    check(applySchema(db), "схема применяется без ошибок");
    check(applySchema(db), "повторное применение схемы идемпотентно (IF NOT EXISTS)");
    insertBaseData(db);

    check(run(db, reservation(1001, 1, 18, 2, 2, "Confirmed")), "обычная бронь стола вставляется");
    check(failsWith(db, reservation(1002, 1, 19, 2, 2, "Confirmed"), "table_overlap"),
          "пересекающаяся бронь того же стола запрещена");
    check(run(db, reservation(1003, 1, 20, 2, 2, "Confirmed")),
          "бронь «встык» (начало = конец предыдущей) разрешена");
    check(run(db, reservation(1004, 2, 18, 2, 2, "Confirmed")), "тот же час на другом столе разрешён");
    check(run(db, reservation(1005, 1, 18, 2, 2, "Cancelled")), "отменённая бронь не блокирует и не конфликтует");

    check(failsWith(db, "UPDATE reservations SET status='Confirmed' WHERE id=1005", "table_overlap"),
          "нельзя «воскресить» отменённую бронь поверх занятого времени (старый баг двойного бронирования)");
    check(run(db, "UPDATE reservations SET status='Cancelled' WHERE id=1001"), "отмена брони проходит");
    check(run(db, "UPDATE reservations SET status='Confirmed' WHERE id=1005"),
          "после освобождения времени отменённую бронь можно восстановить");
    check(failsWith(db, "UPDATE reservations SET table_id=1, start_hour=20 WHERE id=1004", "table_overlap"),
          "перенос брони на занятый стол/время запрещён");

    check(failsWith(db, reservation(1010, 1, 10, 1, 3, "Confirmed"), "party_exceeds_capacity"),
          "гостей больше, чем мест за столом — запрещено");
    check(failsWith(db, "UPDATE reservations SET party_size=9 WHERE id=1003", "party_exceeds_capacity"),
          "увеличить компанию сверх вместимости при редактировании нельзя");

    check(!run(db, reservation(1011, 2, 9, 1, 2, "Confirmed")), "бронь до открытия (9:00) запрещена");
    check(!run(db, reservation(1012, 2, 22, 2, 2, "Confirmed")), "бронь, выходящая за закрытие (22:00 + 2 ч), запрещена");
    check(!run(db, reservation(1013, 2, 12, 5, 2, "Confirmed")), "длительность более 4 часов запрещена");
    check(!run(db, reservation(1014, 2, 12, 0, 2, "Confirmed")), "нулевая длительность запрещена");
    check(!run(db, reservation(1015, 2, 12, 2, 0, "Confirmed")), "0 гостей запрещено");
    check(!run(db, reservation(1016, 2, 12, 2, 2, "Bogus")), "произвольный статус запрещён");
    check(!run(db, "UPDATE reservations SET status='Bogus' WHERE id=1003"), "смена статуса на произвольный запрещена");

    check(!run(db, "INSERT INTO reviews (author_name,review_date,rating,comment) VALUES ('a','2030-01-01',0,'x')"),
          "оценка 0 отклоняется");
    check(!run(db, "INSERT INTO reviews (author_name,review_date,rating,comment) VALUES ('a','2030-01-01',6,'x')"),
          "оценка 6 отклоняется");
    check(run(db, "INSERT INTO reviews (author_name,review_date,rating,comment) VALUES ('a','2030-01-01',5,'ok')"),
          "оценка 5 принимается");

    check(!run(db, "INSERT INTO preorder_items (reservation_id,dish_id,quantity) VALUES (1003,999,1)"),
          "предзаказ несуществующего блюда отклоняется (внешний ключ)");
    check(!run(db, "INSERT INTO preorder_items (reservation_id,dish_id,quantity) VALUES (1003,1,0)"),
          "предзаказ с нулевым количеством отклоняется");
    check(run(db, "INSERT INTO preorder_items (reservation_id,dish_id,quantity) VALUES (1003,1,2)"),
          "корректный предзаказ принимается");
    check(!run(db, "INSERT INTO dishes (name,category,price,weight_grams,calories,prep_time_min,tags,description) "
                   "VALUES ('Бесплатно','Закуски',-5,100,100,5,'','d')"),
          "блюдо с отрицательной ценой отклоняется");
    check(!run(db, "INSERT INTO waitlist (guest_name,guest_phone,visit_date,preferred_hour,party_size) VALUES ('a','1','2030-01-01',12,0)"),
          "лист ожидания: 0 гостей отклоняется");

    sqlite3_close(db);
}

// Схема, которую создавала ПРЕДЫДУЩАЯ версия программы (без CHECK, триггеров и новых колонок)
const char* const kLegacySchema[] = {
    "CREATE TABLE tables (id INTEGER PRIMARY KEY, code TEXT NOT NULL UNIQUE, zone TEXT NOT NULL, capacity INTEGER NOT NULL, grid_x INTEGER NOT NULL, grid_y INTEGER NOT NULL, has_outlet INTEGER NOT NULL DEFAULT 0, is_quiet INTEGER NOT NULL DEFAULT 0, has_sofa INTEGER NOT NULL DEFAULT 0, near_window INTEGER NOT NULL DEFAULT 0, min_deposit REAL NOT NULL DEFAULT 0.0, description TEXT NOT NULL)",
    "CREATE TABLE dishes (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, category TEXT NOT NULL, price REAL NOT NULL, weight_grams INTEGER NOT NULL, calories INTEGER NOT NULL, prep_time_min INTEGER NOT NULL, tags TEXT NOT NULL, description TEXT NOT NULL)",
    "CREATE TABLE reservations (id INTEGER PRIMARY KEY AUTOINCREMENT, booking_code TEXT NOT NULL UNIQUE, guest_name TEXT NOT NULL, guest_phone TEXT NOT NULL, visit_date TEXT NOT NULL, start_hour INTEGER NOT NULL, duration_hours INTEGER NOT NULL, party_size INTEGER NOT NULL, table_id INTEGER NOT NULL, booking_type TEXT NOT NULL, status TEXT NOT NULL, occasion TEXT NOT NULL, comment TEXT NOT NULL DEFAULT '', FOREIGN KEY (table_id) REFERENCES tables(id))",
    "CREATE TABLE preorder_items (id INTEGER PRIMARY KEY AUTOINCREMENT, reservation_id INTEGER NOT NULL, dish_id INTEGER NOT NULL, quantity INTEGER NOT NULL DEFAULT 1, FOREIGN KEY (reservation_id) REFERENCES reservations(id) ON DELETE CASCADE, FOREIGN KEY (dish_id) REFERENCES dishes(id))",
    "CREATE TABLE reviews (id INTEGER PRIMARY KEY AUTOINCREMENT, author_name TEXT NOT NULL, review_date TEXT NOT NULL, rating INTEGER NOT NULL, table_id INTEGER NOT NULL DEFAULT 0, dish_id INTEGER NOT NULL DEFAULT 0, comment TEXT NOT NULL, manager_reply TEXT NOT NULL DEFAULT '')",
    "CREATE TABLE waitlist (id INTEGER PRIMARY KEY AUTOINCREMENT, guest_name TEXT NOT NULL, guest_phone TEXT NOT NULL, visit_date TEXT NOT NULL, preferred_hour INTEGER NOT NULL, party_size INTEGER NOT NULL, comment TEXT NOT NULL DEFAULT '')"
};

void testLegacyMigration() {
    std::cout << "\n== Миграция БД старой версии ==\n";
    sqlite3* db = nullptr;
    sqlite3_open(":memory:", &db);
    run(db, "PRAGMA foreign_keys = ON");
    for (const char* sql : kLegacySchema) run(db, sql);
    run(db, "INSERT INTO tables VALUES (1,'W1','Окна',2,0,0,1,1,0,1,0.0,'d')");
    run(db, "INSERT INTO dishes (id,name,category,price,weight_grams,calories,prep_time_min,tags,description) "
            "VALUES (1,'Старое блюдо','Закуски',100,100,100,5,'легкое','d')");
    // В старой схеме можно было вставить даже пересекающиеся брони — данные пользователя должны пережить миграцию
    run(db, "INSERT INTO reservations VALUES (1001,'BK-1001','Старый гость','+7 900 000-00-00','2030-01-01',18,2,2,1,'Online','Confirmed','Свидание','')");
    run(db, "INSERT INTO reservations VALUES (1002,'BK-1002','Старый гость 2','+7 900 000-00-01','2030-01-01',19,2,2,1,'Online','Cancelled','Свидание','')");
    run(db, "INSERT INTO preorder_items (reservation_id,dish_id,quantity) VALUES (1001,1,2)");

    check(!columnExists(db, "reservations", "deposit_paid") && !columnExists(db, "dishes", "allergens"),
          "в старой БД новых колонок нет");
    check(applySchema(db), "миграция применяется без ошибок");
    check(columnExists(db, "reservations", "deposit_paid"), "добавлена колонка reservations.deposit_paid");
    check(columnExists(db, "dishes", "allergens"), "добавлена колонка dishes.allergens");
    check(queryInt(db, "SELECT COUNT(*) FROM reservations") == 2 && queryInt(db, "SELECT COUNT(*) FROM preorder_items") == 1,
          "данные броней и предзаказов сохранились");
    check(queryText(db, "SELECT guest_name FROM reservations WHERE id=1001") == "Старый гость", "кириллица в данных не повреждена");
    check(queryInt(db, "SELECT deposit_paid FROM reservations WHERE id=1001") == 0, "депозит у старых броней = 0");
    check(applySchema(db), "повторный запуск миграции безопасен");
    check(failsWith(db, "UPDATE reservations SET status='Confirmed' WHERE id=1002", "table_overlap"),
          "триггеры защищают и старую БД: отменённую бронь нельзя восстановить поверх занятого времени");
    check(failsWith(db, "INSERT INTO reviews (author_name,review_date,rating,comment) VALUES ('a','2030-01-01',9,'x')", "invalid_rating"),
          "триггер оценки работает в старой БД (там не было CHECK)");
    check(queryText(db, "PRAGMA integrity_check") == "ok", "integrity_check = ok");
    check(queryInt(db, "SELECT COUNT(*) FROM pragma_foreign_key_check") == 0, "нарушений внешних ключей нет");
    sqlite3_close(db);
}

bool copyFile(const std::string& from, const std::string& to) {
    std::ifstream in(from, std::ios::binary);
    if (!in) return false;
    std::ofstream out(to, std::ios::binary | std::ios::trunc);
    out << in.rdbuf();
    return static_cast<bool>(out);
}

void testExistingDatabaseCopy(const std::string& path) {
    std::cout << "\n== Миграция реального файла БД: " << path << " (на копии) ==\n";
    const std::string tmp = "/tmp/restaurant_migration_check.db";
    std::remove(tmp.c_str());
    if (!copyFile(path, tmp)) {
        check(false, "файл БД открывается для копирования");
        return;
    }
    sqlite3* db = nullptr;
    sqlite3_open(tmp.c_str(), &db);
    const int resBefore = queryInt(db, "SELECT COUNT(*) FROM reservations");
    const int dishesBefore = queryInt(db, "SELECT COUNT(*) FROM dishes");
    check(applySchema(db), "миграция применяется к реальной БД");
    check(queryInt(db, "SELECT COUNT(*) FROM reservations") == resBefore &&
          queryInt(db, "SELECT COUNT(*) FROM dishes") == dishesBefore, "количество записей не изменилось");
    check(queryText(db, "PRAGMA integrity_check") == "ok", "integrity_check = ok");
    check(queryInt(db, "SELECT COUNT(*) FROM pragma_foreign_key_check") == 0, "нарушений внешних ключей нет");
    check(queryInt(db, "SELECT COUNT(*) FROM reservations r JOIN tables t ON t.id=r.table_id WHERE r.party_size > t.capacity") == 0,
          "в данных нет броней, превышающих вместимость стола");
    check(queryInt(db, "SELECT COUNT(*) FROM reservations a JOIN reservations b ON a.id < b.id AND a.table_id=b.table_id "
                       "AND a.visit_date=b.visit_date AND a.status IN ('Confirmed','Seated') AND b.status IN ('Confirmed','Seated') "
                       "AND a.start_hour < b.start_hour + b.duration_hours AND a.start_hour + a.duration_hours > b.start_hour") == 0,
          "в данных нет двойных броней");
    sqlite3_close(db);
    std::remove(tmp.c_str());
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "Проверка слоя базы данных SQLite (схема, миграция, триггеры)\n";
    testFreshDatabase();
    testLegacyMigration();
    if (argc > 1) testExistingDatabaseCopy(argv[1]);

    std::cout << "\nПроверок: " << g_checks << ", провалено: " << g_failed << "\n";
    if (g_failed == 0) {
        std::cout << "ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n";
        return 0;
    }
    return 1;
}
