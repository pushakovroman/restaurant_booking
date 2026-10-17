#ifndef SCHEMA_SQL_HPP
#define SCHEMA_SQL_HPP

// Схема БД, миграции и триггеры целостности. Файл НЕ зависит от Qt:
// его использует и приложение (DatabaseManager), и автотест tests/test_sqlite_db.cpp,
// поэтому тест проверяет ровно тот SQL, который выполняется в программе.
//
// Рабочие часы зала (10:00-23:00) продублированы в SQL-строках ниже макросами.
// При их изменении поменяйте RB_OPEN_HOUR_SQL / RB_CLOSE_HOUR_SQL и значения в DatabaseManager.hpp.

#define RB_OPEN_HOUR_SQL "10"
#define RB_CLOSE_HOUR_SQL "23"

namespace Restaurant {
namespace Schema {

constexpr int kOpenHour = 10;
constexpr int kCloseHour = 23;

// 1. Таблицы (для новой БД — сразу с CHECK-ограничениями и новыми колонками)
inline constexpr const char* kTableStatements[] = {
    R"SQL(CREATE TABLE IF NOT EXISTS tables (
        id INTEGER PRIMARY KEY,
        code TEXT NOT NULL UNIQUE,
        zone TEXT NOT NULL,
        capacity INTEGER NOT NULL CHECK (capacity > 0),
        grid_x INTEGER NOT NULL,
        grid_y INTEGER NOT NULL,
        has_outlet INTEGER NOT NULL DEFAULT 0,
        is_quiet INTEGER NOT NULL DEFAULT 0,
        has_sofa INTEGER NOT NULL DEFAULT 0,
        near_window INTEGER NOT NULL DEFAULT 0,
        min_deposit REAL NOT NULL DEFAULT 0.0 CHECK (min_deposit >= 0),
        description TEXT NOT NULL))SQL",

    R"SQL(CREATE TABLE IF NOT EXISTS dishes (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        category TEXT NOT NULL,
        price REAL NOT NULL CHECK (price > 0),
        weight_grams INTEGER NOT NULL CHECK (weight_grams > 0),
        calories INTEGER NOT NULL CHECK (calories >= 0),
        prep_time_min INTEGER NOT NULL CHECK (prep_time_min > 0),
        tags TEXT NOT NULL,
        description TEXT NOT NULL,
        allergens TEXT NOT NULL DEFAULT ''))SQL",

    R"SQL(CREATE TABLE IF NOT EXISTS reservations (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        booking_code TEXT NOT NULL UNIQUE,
        guest_name TEXT NOT NULL,
        guest_phone TEXT NOT NULL,
        visit_date TEXT NOT NULL CHECK (visit_date GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'),
        start_hour INTEGER NOT NULL CHECK (start_hour BETWEEN )SQL" RB_OPEN_HOUR_SQL R"SQL( AND 22),
        duration_hours INTEGER NOT NULL CHECK (duration_hours BETWEEN 1 AND 4),
        party_size INTEGER NOT NULL CHECK (party_size >= 1),
        table_id INTEGER NOT NULL,
        booking_type TEXT NOT NULL CHECK (booking_type IN ('Online', 'WalkIn')),
        status TEXT NOT NULL CHECK (status IN ('Confirmed', 'Seated', 'Completed', 'Cancelled')),
        occasion TEXT NOT NULL,
        comment TEXT NOT NULL DEFAULT '',
        deposit_paid REAL NOT NULL DEFAULT 0 CHECK (deposit_paid >= 0),
        CHECK (start_hour + duration_hours <= )SQL" RB_CLOSE_HOUR_SQL R"SQL(),
        FOREIGN KEY (table_id) REFERENCES tables(id)))SQL",

    R"SQL(CREATE TABLE IF NOT EXISTS preorder_items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        reservation_id INTEGER NOT NULL,
        dish_id INTEGER NOT NULL,
        quantity INTEGER NOT NULL DEFAULT 1 CHECK (quantity > 0),
        FOREIGN KEY (reservation_id) REFERENCES reservations(id) ON DELETE CASCADE,
        FOREIGN KEY (dish_id) REFERENCES dishes(id)))SQL",

    R"SQL(CREATE TABLE IF NOT EXISTS reviews (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        author_name TEXT NOT NULL,
        review_date TEXT NOT NULL,
        rating INTEGER NOT NULL CHECK (rating >= 1 AND rating <= 5),
        table_id INTEGER NOT NULL DEFAULT 0,
        dish_id INTEGER NOT NULL DEFAULT 0,
        comment TEXT NOT NULL,
        manager_reply TEXT NOT NULL DEFAULT ''))SQL",

    R"SQL(CREATE TABLE IF NOT EXISTS waitlist (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        guest_name TEXT NOT NULL,
        guest_phone TEXT NOT NULL,
        visit_date TEXT NOT NULL,
        preferred_hour INTEGER NOT NULL,
        party_size INTEGER NOT NULL CHECK (party_size >= 1),
        comment TEXT NOT NULL DEFAULT ''))SQL"
};

// 2. Миграция БД старой версии: колонки, которых там нет (ALTER TABLE ... ADD COLUMN)
struct ColumnMigration {
    const char* table;
    const char* column;
    const char* ddl;
};

inline constexpr ColumnMigration kColumnMigrations[] = {
    {"reservations", "deposit_paid", "ALTER TABLE reservations ADD COLUMN deposit_paid REAL NOT NULL DEFAULT 0"},
    {"dishes",       "allergens",    "ALTER TABLE dishes ADD COLUMN allergens TEXT NOT NULL DEFAULT ''"}
};

// 3. Индексы и триггеры. Триггеры защищают данные на уровне БД, поэтому действуют
//    и на старые базы (где CHECK-ограничений нет), и при записи не из приложения.
inline constexpr const char* kIndexAndTriggerStatements[] = {
    "CREATE INDEX IF NOT EXISTS idx_res_table_date ON reservations(table_id, visit_date, status)",
    "CREATE INDEX IF NOT EXISTS idx_res_date ON reservations(visit_date)",
    "CREATE INDEX IF NOT EXISTS idx_preorder_res ON preorder_items(reservation_id)",
    "CREATE INDEX IF NOT EXISTS idx_reviews_table ON reviews(table_id)",
    "CREATE INDEX IF NOT EXISTS idx_reviews_dish ON reviews(dish_id)",

    // Запрет двойного бронирования стола (пересечение интервалов у активных броней)
    R"SQL(CREATE TRIGGER IF NOT EXISTS trg_res_no_overlap_ins
        BEFORE INSERT ON reservations
        WHEN NEW.status IN ('Confirmed', 'Seated')
         AND EXISTS (SELECT 1 FROM reservations r
                     WHERE r.table_id = NEW.table_id AND r.visit_date = NEW.visit_date
                       AND r.status IN ('Confirmed', 'Seated')
                       AND r.id <> COALESCE(NEW.id, -1)
                       AND NEW.start_hour < r.start_hour + r.duration_hours
                       AND NEW.start_hour + NEW.duration_hours > r.start_hour)
        BEGIN SELECT RAISE(ABORT, 'table_overlap'); END)SQL",

    R"SQL(CREATE TRIGGER IF NOT EXISTS trg_res_no_overlap_upd
        BEFORE UPDATE OF status, table_id, visit_date, start_hour, duration_hours ON reservations
        WHEN NEW.status IN ('Confirmed', 'Seated')
         AND EXISTS (SELECT 1 FROM reservations r
                     WHERE r.table_id = NEW.table_id AND r.visit_date = NEW.visit_date
                       AND r.status IN ('Confirmed', 'Seated')
                       AND r.id <> NEW.id
                       AND NEW.start_hour < r.start_hour + r.duration_hours
                       AND NEW.start_hour + NEW.duration_hours > r.start_hour)
        BEGIN SELECT RAISE(ABORT, 'table_overlap'); END)SQL",

    // Вместимость стола
    R"SQL(CREATE TRIGGER IF NOT EXISTS trg_res_capacity_ins
        BEFORE INSERT ON reservations
        WHEN NEW.party_size > COALESCE((SELECT capacity FROM tables WHERE id = NEW.table_id), 2147483647)
        BEGIN SELECT RAISE(ABORT, 'party_exceeds_capacity'); END)SQL",

    R"SQL(CREATE TRIGGER IF NOT EXISTS trg_res_capacity_upd
        BEFORE UPDATE OF table_id, party_size ON reservations
        WHEN NEW.party_size > COALESCE((SELECT capacity FROM tables WHERE id = NEW.table_id), 2147483647)
        BEGIN SELECT RAISE(ABORT, 'party_exceeds_capacity'); END)SQL",

    // Допустимые значения (рабочие часы, длительность, статусы)
    R"SQL(CREATE TRIGGER IF NOT EXISTS trg_res_valid_ins
        BEFORE INSERT ON reservations
        WHEN NEW.start_hour < )SQL" RB_OPEN_HOUR_SQL R"SQL(
          OR NEW.duration_hours < 1 OR NEW.duration_hours > 4
          OR NEW.start_hour + NEW.duration_hours > )SQL" RB_CLOSE_HOUR_SQL R"SQL(
          OR NEW.party_size < 1
          OR NEW.status NOT IN ('Confirmed', 'Seated', 'Completed', 'Cancelled')
          OR NEW.booking_type NOT IN ('Online', 'WalkIn')
        BEGIN SELECT RAISE(ABORT, 'invalid_reservation'); END)SQL",

    R"SQL(CREATE TRIGGER IF NOT EXISTS trg_res_valid_upd
        BEFORE UPDATE OF start_hour, duration_hours, party_size, status, booking_type ON reservations
        WHEN NEW.start_hour < )SQL" RB_OPEN_HOUR_SQL R"SQL(
          OR NEW.duration_hours < 1 OR NEW.duration_hours > 4
          OR NEW.start_hour + NEW.duration_hours > )SQL" RB_CLOSE_HOUR_SQL R"SQL(
          OR NEW.party_size < 1
          OR NEW.status NOT IN ('Confirmed', 'Seated', 'Completed', 'Cancelled')
          OR NEW.booking_type NOT IN ('Online', 'WalkIn')
        BEGIN SELECT RAISE(ABORT, 'invalid_reservation'); END)SQL",

    // Оценка отзыва 1..5 (в старых БД, созданных программой, CHECK не было)
    R"SQL(CREATE TRIGGER IF NOT EXISTS trg_reviews_rating_ins
        BEFORE INSERT ON reviews
        WHEN NEW.rating < 1 OR NEW.rating > 5
        BEGIN SELECT RAISE(ABORT, 'invalid_rating'); END)SQL",

    R"SQL(CREATE TRIGGER IF NOT EXISTS trg_reviews_rating_upd
        BEFORE UPDATE OF rating ON reviews
        WHEN NEW.rating < 1 OR NEW.rating > 5
        BEGIN SELECT RAISE(ABORT, 'invalid_rating'); END)SQL"
};

} // namespace Schema
} // namespace Restaurant

#endif // SCHEMA_SQL_HPP
