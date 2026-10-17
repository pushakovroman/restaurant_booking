#include "DatabaseManager.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QDate>
#include <QTime>
#include <QSet>
#include <QDebug>
#include <algorithm>
#include <cmath>

namespace Restaurant {

namespace {

const char* const kConnectionName = "restaurant_sql_conn";

const char* const kReservationColumns =
    "id, booking_code, guest_name, guest_phone, visit_date, start_hour, duration_hours, "
    "party_size, table_id, booking_type, status, occasion, comment, deposit_paid";

const char* const kDishColumns =
    "id, name, category, price, weight_grams, calories, prep_time_min, tags, description, allergens";

const char* const kTableColumns =
    "id, code, zone, capacity, grid_x, grid_y, has_outlet, is_quiet, has_sofa, near_window, min_deposit, description";

DishInfo dishFromQuery(const QSqlQuery& q) {
    DishInfo d;
    d.id = q.value(0).toInt();
    d.name = q.value(1).toString();
    d.category = q.value(2).toString();
    d.price = q.value(3).toDouble();
    d.weightGrams = q.value(4).toInt();
    d.calories = q.value(5).toInt();
    d.prepTimeMin = q.value(6).toInt();
    d.tags = q.value(7).toString().split(",", Qt::SkipEmptyParts);
    d.description = q.value(8).toString();
    d.allergens = q.value(9).toString().split(",", Qt::SkipEmptyParts);
    return d;
}

TableInfo tableFromQuery(const QSqlQuery& q) {
    TableInfo t;
    t.id = q.value(0).toInt();
    t.code = q.value(1).toString();
    t.zone = q.value(2).toString();
    t.capacity = q.value(3).toInt();
    t.gridX = q.value(4).toInt();
    t.gridY = q.value(5).toInt();
    t.hasOutlet = q.value(6).toInt() != 0;
    t.isQuiet = q.value(7).toInt() != 0;
    t.hasSofa = q.value(8).toInt() != 0;
    t.nearWindow = q.value(9).toInt() != 0;
    t.minDeposit = q.value(10).toDouble();
    t.description = q.value(11).toString();
    return t;
}

QString todayString() {
    return QDate::currentDate().toString("yyyy-MM-dd");
}

// Переводит текст ошибки SQLite (из триггеров) в понятное сообщение
QString describeDbError(const QString& text) {
    if (text.contains("table_overlap"))          return QString("Стол уже занят на это время (пересечение с другой бронью).");
    if (text.contains("party_exceeds_capacity")) return QString("Количество гостей превышает вместимость стола.");
    if (text.contains("invalid_reservation"))    return QString("Некорректные параметры брони (время, длительность, статус или тип).");
    if (text.contains("invalid_rating"))         return QString("Оценка должна быть от 1 до 5.");
    return QString("Ошибка базы данных: ") + text;
}

} // namespace

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::openDatabase(const QString& dbFilePath) {
    lastError_.clear();

    QFileInfo fi(dbFilePath);
    if (!fi.dir().exists() && !QDir().mkpath(fi.dir().absolutePath())) {
        lastError_ = QString("Не удалось создать папку для базы данных: %1").arg(fi.dir().absolutePath());
        qWarning() << lastError_;
        return false;
    }

    if (QSqlDatabase::contains(kConnectionName)) {
        db_ = QSqlDatabase::database(kConnectionName, false);
    } else {
        db_ = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
    }
    if (db_.isOpen()) db_.close();
    db_.setDatabaseName(dbFilePath);
    // Если файл БД на мгновение занят другим процессом — ждём, а не падаем сразу
    db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");

    if (!db_.open()) {
        lastError_ = db_.lastError().text();
        qWarning() << "Не удалось открыть базу данных SQLite:" << lastError_;
        return false;
    }

    if (!initializeSchemaAndSeedData()) {
        qWarning() << "Не удалось подготовить схему базы данных:" << lastError_;
        db_.close();
        return false;
    }
    return true;
}

void DatabaseManager::closeDatabase() {
    if (db_.isOpen()) {
        db_.close();
    }
}

bool DatabaseManager::columnExists(const QString& table, const QString& column) const {
    QSqlQuery q(db_);
    if (!q.exec(QString("PRAGMA table_info(%1)").arg(table))) return false;
    while (q.next()) {
        if (q.value(1).toString().compare(column, Qt::CaseInsensitive) == 0) return true;
    }
    return false;
}

bool DatabaseManager::initializeSchemaAndSeedData() {
    QSqlQuery q(db_);
    if (!q.exec("PRAGMA foreign_keys = ON")) {
        lastError_ = q.lastError().text();
        return false;
    }

    for (const char* sql : Schema::kTableStatements) {
        if (!q.exec(QString::fromUtf8(sql))) {
            lastError_ = q.lastError().text();
            return false;
        }
    }

    // Миграция баз, созданных старой версией программы (добавляем недостающие колонки)
    const bool allergensColumnWasMissing = !columnExists("dishes", "allergens");
    for (const auto& m : Schema::kColumnMigrations) {
        if (!columnExists(QString::fromUtf8(m.table), QString::fromUtf8(m.column))) {
            if (!q.exec(QString::fromUtf8(m.ddl))) {
                lastError_ = q.lastError().text();
                return false;
            }
        }
    }

    for (const char* sql : Schema::kIndexAndTriggerStatements) {
        if (!q.exec(QString::fromUtf8(sql))) {
            lastError_ = q.lastError().text();
            return false;
        }
    }

    int tablesCount = 0;
    if (q.exec("SELECT COUNT(*) FROM tables") && q.next()) {
        tablesCount = q.value(0).toInt();
    }
    q.finish();

    if (tablesCount == 0) {
        return seedDemoData();
    }

    if (allergensColumnWasMissing) {
        // Старая БД: заполняем аллергены у блюд исходного меню (по описанию состава)
        struct Backfill { int id; const char* name; const char* allergens; };
        static const Backfill kBackfill[] = {
            {1,  "Тартар из лосося с манго и авокадо", "рыба,соя"},
            {2,  "Буррата с узбекскими томатами и песто", "молоко,орехи"},
            {3,  "Крем-суп из белых грибов с трюфельным маслом", "молоко"},
            {4,  "Том Ям с тигровыми креветками и кальмаром", "ракообразные,моллюски"},
            {6,  "Филе палтуса на подушке из зеленой спаржи", "рыба"},
            {7,  "Ризотто с лесными лисичками и страчателлой", "молоко"},
            {9,  "Паста Тальятелле с камчатским крабом", "глютен,яйца,ракообразные"},
            {10, "Фисташковый тирамису с малиновым кули", "молоко,глютен,яйца,орехи"}
        };
        for (const auto& b : kBackfill) {
            QSqlQuery upd(db_);
            upd.prepare("UPDATE dishes SET allergens = ? WHERE id = ? AND name = ? AND allergens = ''");
            upd.addBindValue(QString::fromUtf8(b.allergens));
            upd.addBindValue(b.id);
            upd.addBindValue(QString::fromUtf8(b.name));
            upd.exec();
        }
    }
    return true;
}

bool DatabaseManager::seedDemoData() {
    QSqlQuery tx(db_);
    if (!tx.exec("BEGIN IMMEDIATE")) {
        lastError_ = tx.lastError().text();
        return false;
    }
    auto fail = [this](const QSqlQuery& failed) {
        lastError_ = failed.lastError().text();
        QSqlQuery rb(db_);
        rb.exec("ROLLBACK");
        return false;
    };

    const std::vector<TableInfo> defaultTables = {
        {1,  "W1", "Панорамные окна", 2,  0, 0, true,  true,  false, true,  0.0,    "Уютный столик на двоих у панорамного окна с видом на огни набережной"},
        {2,  "W2", "Панорамные окна", 2,  1, 0, false, true,  false, true,  0.0,    "Романтический столик у окна с живыми цветами и мягким светом"},
        {3,  "T1", "Летняя терраса",  4,  2, 0, false, false, true,  true,  0.0,    "Столик на теплой панорамной веранде с пледами и плетеными диванами"},
        {4,  "T2", "Летняя терраса",  6,  3, 0, true,  false, true,  true,  0.0,    "Большой стол на террасе для дружной компании с видом на сад"},
        {5,  "M1", "Основной зал",    4,  0, 1, true,  false, true,  false, 0.0,    "Комфортный стол в основном зале с велюровым диваном и розеткой"},
        {6,  "M2", "Основной зал",    4,  1, 1, false, false, false, false, 0.0,    "Классический круглый стол в центре зала рядом с винной витриной"},
        {7,  "M3", "Основной зал",    6,  2, 1, true,  false, true,  false, 0.0,    "Просторный стол для компании или семьи в центральной части зала"},
        {8,  "B1", "Барная зона",     2,  3, 1, true,  false, false, false, 0.0,    "Высокая посадка у контактного бара с видом на работу шеф-бармена"},
        {9,  "V1", "VIP-каминная",    6,  0, 2, true,  true,  true,  false, 5000.0, "Приватная ложа в VIP-зале у дровяного камина со звукоизоляцией"},
        {10, "V2", "VIP-каминная",    10, 1, 2, true,  true,  true,  false, 8000.0, "Большой банкетный стол из массива дуба для переговоров и торжеств"},
        {11, "M4", "Основной зал",    4,  2, 2, true,  true,  true,  false, 0.0,    "Тихий семейный уголок с мягкими креслами, вдали от входной группы"},
        {12, "B2", "Барная зона",     2,  3, 2, true,  false, false, false, 0.0,    "Камерный коктейльный столик рядом с открытой кухней и сомелье"}
    };

    for (const auto& t : defaultTables) {
        QSqlQuery ins(db_);
        ins.prepare(QString("INSERT INTO tables (%1) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)").arg(kTableColumns));
        ins.addBindValue(t.id);
        ins.addBindValue(t.code);
        ins.addBindValue(t.zone);
        ins.addBindValue(t.capacity);
        ins.addBindValue(t.gridX);
        ins.addBindValue(t.gridY);
        ins.addBindValue(t.hasOutlet ? 1 : 0);
        ins.addBindValue(t.isQuiet ? 1 : 0);
        ins.addBindValue(t.hasSofa ? 1 : 0);
        ins.addBindValue(t.nearWindow ? 1 : 0);
        ins.addBindValue(t.minDeposit);
        ins.addBindValue(t.description);
        if (!ins.exec()) return fail(ins);
    }

    // Аллергены указаны по описанию состава блюд — перед использованием в ресторане их должен подтвердить шеф-повар
    const std::vector<DishInfo> defaultDishes = {
        {1,  "Тартар из лосося с манго и авокадо", "Закуски", 890.0,  190, 310, 12, {"морепродукты", "без глютена", "легкое", "хит шефа"}, "Охлажденный фарерский лосось, спелое манго, мусс из авокадо и соус понзу", {"рыба", "соя"}},
        {2,  "Буррата с узбекскими томатами и песто", "Закуски", 850.0,  240, 390, 10, {"вегетарианское", "без глютена", "хит шефа"}, "Фермерская сливочная буррата, сладкие томаты, кедровый орех и базиликовый песто", {"молоко", "орехи"}},
        {3,  "Крем-суп из белых грибов с трюфельным маслом", "Супы", 640.0,  300, 340, 15, {"вегетарианское"}, "Ароматные белые грибы, сливки 22%, чипсы из пармезана и капля белого трюфеля", {"молоко"}},
        {4,  "Том Ям с тигровыми креветками и кальмаром", "Супы", 920.0,  380, 410, 18, {"морепродукты", "острое", "без глютена"}, "Пряный бульон на кокосовом молоке с лемонграссом, галангалом, креветками и рисом жасмин", {"ракообразные", "моллюски"}},
        {5,  "Стейк Рибай Prime с перечным демигласом", "Горячее", 2450.0, 320, 760, 25, {"без глютена", "хит шефа"}, "Мраморная говядина зернового откорма, запеченный чеснок, розмарин и соус из трех перцев", {}},
        {6,  "Филе палтуса на подушке из зеленой спаржи", "Горячее", 1490.0, 260, 420, 20, {"морепродукты", "без глютена", "легкое"}, "Нежное филе северного палтуса су-вид, молодая спаржа на гриле и шафрановый велюте", {"рыба"}},
        {7,  "Ризотто с лесными лисичками и страчателлой", "Горячее", 980.0,  290, 540, 20, {"вегетарианское", "без глютена"}, "Рис карнароли, обжаренные лисички, сливочная страчателла и выдержанный пармезан", {"молоко"}},
        {8,  "Утиная грудка Магре с грушей в пряном вине", "Горячее", 1350.0, 270, 590, 22, {"хит шефа"}, "Розовая утиная грудка с хрустящей корочкой, томленая груша и ягодный взвар", {}},
        {9,  "Паста Тальятелле с камчатским крабом", "Горячее", 1620.0, 280, 560, 18, {"морепродукты", "хит шефа"}, "Домашняя паста ручной работы, первая фаланга камчатского краба и соус биск", {"глютен", "яйца", "ракообразные"}},
        {10, "Фисташковый тирамису с малиновым кули", "Десерты", 590.0,  160, 430, 10, {"вегетарианское", "хит шефа"}, "Воздушный крем маскарпоне с иранской фисташковой пастой, савоярди и свежая малина", {"молоко", "глютен", "яйца", "орехи"}},
        {11, "Сорбет Манго-Маракуйя с семенами чиа", "Десерты", 420.0,  140, 165, 7, {"вегетарианское", "без глютена", "легкое"}, "Освежающий натуральный сорбет без сахара со свежей мятой и чиа", {}},
        {12, "Авторский лимонад «Лаванда — Дикая Ежевика»", "Напитки", 380.0,  350, 110, 5, {"вегетарианское", "без глютена", "легкое"}, "Настой крымской лаванды, пюре лесной ежевики, сок лайма и родниковая вода", {}},
        {13, "Чайник согревающего чая «Облепиха — Имбирь — Груша»", "Напитки", 490.0,  600, 140, 8, {"вегетарианское", "без глютена", "хит шефа"}, "Таежная облепиха, слайсы медовой груши, свежий корень имбиря и корица", {}},
        {14, "Безалкогольный коктейль «Botanical Spritz»", "Напитки", 460.0,  280, 95,  6, {"вегетарианское", "без глютена", "легкое"}, "Безалкогольный аперитив на травах, грейпфрутовый кордиал и тоник", {}}
    };

    for (const auto& d : defaultDishes) {
        QSqlQuery ins(db_);
        ins.prepare(QString("INSERT INTO dishes (%1) VALUES (?,?,?,?,?,?,?,?,?,?)").arg(kDishColumns));
        ins.addBindValue(d.id);
        ins.addBindValue(d.name);
        ins.addBindValue(d.category);
        ins.addBindValue(d.price);
        ins.addBindValue(d.weightGrams);
        ins.addBindValue(d.calories);
        ins.addBindValue(d.prepTimeMin);
        ins.addBindValue(d.tags.join(","));
        ins.addBindValue(d.description);
        ins.addBindValue(d.allergens.join(","));
        if (!ins.exec()) return fail(ins);
    }

    // Демо-брони привязаны к текущей дате (раньше дата была зашита в код)
    const QString today = todayString();
    struct SeedReservation {
        int id; const char* name; const char* phone; int startHour; int durationHours; int partySize;
        int tableId; const char* type; const char* status; const char* occasion; const char* comment; double deposit;
    };
    const SeedReservation seedReservations[] = {
        {1001, "Алексей Смирнов", "+7 (916) 555-12-34", 19, 2, 2, 1, "Online", "Confirmed", "Свидание", "Годовщина знакомства, просьба поставить свечу и принести комплимент", 0.0},
        {1002, "Екатерина Волкова", "+7 (926) 777-88-99", 18, 3, 5, 9, "Online", "Seated", "Деловая встреча", "Переговоры с партнерами, нужен тихий стол и 2 меню без цен", 5000.0},
        {1003, "Дмитрий Орлов", "+7 (903) 111-22-33", 19, 2, 3, 5, "WalkIn", "Seated", "Обычный визит", "Гости зашли с улицы, нужен детский стульчик", 0.0},
        {1004, "Мария Соколова", "+7 (999) 444-55-66", 20, 2, 4, 3, "Online", "Confirmed", "День рождения", "Аллергия на арахис у одного гостя, вынести десерт со свечой в 21:00", 0.0}
    };
    for (const auto& r : seedReservations) {
        QSqlQuery ins(db_);
        ins.prepare(QString("INSERT INTO reservations (%1) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)").arg(kReservationColumns));
        ins.addBindValue(r.id);
        ins.addBindValue(QString("BK-%1").arg(r.id));
        ins.addBindValue(QString::fromUtf8(r.name));
        ins.addBindValue(QString::fromUtf8(r.phone));
        ins.addBindValue(today);
        ins.addBindValue(r.startHour);
        ins.addBindValue(r.durationHours);
        ins.addBindValue(r.partySize);
        ins.addBindValue(r.tableId);
        ins.addBindValue(QString::fromUtf8(r.type));
        ins.addBindValue(QString::fromUtf8(r.status));
        ins.addBindValue(QString::fromUtf8(r.occasion));
        ins.addBindValue(QString::fromUtf8(r.comment));
        ins.addBindValue(r.deposit);
        if (!ins.exec()) return fail(ins);
    }

    struct SeedPreorder { int reservationId; int dishId; int quantity; };
    const SeedPreorder seedPreorders[] = {
        {1001, 1, 1}, {1001, 6, 1}, {1001, 8, 1}, {1001, 10, 2},
        {1002, 2, 2}, {1002, 5, 3}, {1002, 13, 2},
        {1003, 7, 2}, {1003, 12, 3},
        {1004, 9, 2}, {1004, 10, 2}, {1004, 14, 4}
    };
    for (const auto& p : seedPreorders) {
        QSqlQuery ins(db_);
        ins.prepare("INSERT INTO preorder_items (reservation_id, dish_id, quantity) VALUES (?,?,?)");
        ins.addBindValue(p.reservationId);
        ins.addBindValue(p.dishId);
        ins.addBindValue(p.quantity);
        if (!ins.exec()) return fail(ins);
    }

    struct SeedReview {
        const char* author; int daysAgo; int rating; int tableId; int dishId; const char* comment; const char* reply;
    };
    const SeedReview seedReviews[] = {
        {"Алина К.", 4, 5, 1, 1, "Потрясающий столик W1 прямо у окна! Тартар из лосося с манго просто тает во рту.", "Благодарим за теплые слова, Алина! Будем рады видеть вас снова за вашим любимым столиком."},
        {"Игорь В.", 3, 5, 9, 5, "Проводили деловой ужин в VIP-зале у камина (стол V1). Тихо, солидно, стейк Рибай идеальной прожарки Medium.", "Спасибо за высокую оценку кухни и сервиса, Игорь!"},
        {"Светлана М.", 2, 4, 3, 9, "Паста с крабом восхитительная, на веранде очень уютно благодаря пледам. Хотелось бы чуть быстрее подачу напитков.", "Светлана, спасибо за обратную связь! Мы усилили смену барменов в вечерние часы."},
        {"Максим П.", 1, 5, 2, 6, "Отмечали свидание за столиком W2. Палтус на спарже и фисташковый тирамису — 10 из 10!", "Максим, рады, что ваш вечер прошел великолепно!"}
    };
    for (const auto& r : seedReviews) {
        QSqlQuery ins(db_);
        ins.prepare("INSERT INTO reviews (author_name, review_date, rating, table_id, dish_id, comment, manager_reply) "
                    "VALUES (?,?,?,?,?,?,?)");
        ins.addBindValue(QString::fromUtf8(r.author));
        ins.addBindValue(QDate::currentDate().addDays(-r.daysAgo).toString("yyyy-MM-dd"));
        ins.addBindValue(r.rating);
        ins.addBindValue(r.tableId);
        ins.addBindValue(r.dishId);
        ins.addBindValue(QString::fromUtf8(r.comment));
        ins.addBindValue(QString::fromUtf8(r.reply));
        if (!ins.exec()) return fail(ins);
    }

    QSqlQuery commit(db_);
    if (!commit.exec("COMMIT")) return fail(commit);
    return true;
}

bool DatabaseManager::isValidGuestName(const QString& name) {
    const QString n = name.simplified();
    if (n.size() < 2 || n.size() > 80) return false;
    for (int i = 0; i < n.size(); ++i) {
        if (n.at(i).isLetter()) return true;
    }
    return false;
}

bool DatabaseManager::isValidPhone(const QString& phone) {
    const QString p = phone.trimmed();
    if (p.isEmpty() || p.size() > 25) return false;
    int digits = 0;
    for (int i = 0; i < p.size(); ++i) {
        const QChar ch = p.at(i);
        if (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) { ++digits; continue; }
        if (ch == QLatin1Char('+') && i == 0) continue;
        if (ch == QLatin1Char(' ') || ch == QLatin1Char('-') || ch == QLatin1Char('(') ||
            ch == QLatin1Char(')') || ch == QLatin1Char('.')) continue;
        return false;
    }
    return digits >= 7 && digits <= 15;
}

bool DatabaseManager::isValidDateString(const QString& date) {
    const QDate d = QDate::fromString(date, "yyyy-MM-dd");
    return d.isValid() && d.toString("yyyy-MM-dd") == date;
}

// ---------------------------------------------------------------- Столики

std::vector<TableInfo> DatabaseManager::getAllTables() const {
    std::vector<TableInfo> list;
    QSqlQuery q(db_);
    q.exec(QString("SELECT %1 FROM tables ORDER BY id ASC").arg(kTableColumns));
    while (q.next()) {
        list.push_back(tableFromQuery(q));
    }
    return list;
}

TableInfo DatabaseManager::getTableById(int id) const {
    QSqlQuery q(db_);
    q.prepare(QString("SELECT %1 FROM tables WHERE id = ?").arg(kTableColumns));
    q.addBindValue(id);
    if (q.exec() && q.next()) {
        return tableFromQuery(q);
    }
    return TableInfo{};
}

std::vector<DatabaseManager::ActiveSlot> DatabaseManager::loadActiveSlots(const QString& date) const {
    std::vector<ActiveSlot> slots;
    QSqlQuery q(db_);
    q.prepare("SELECT table_id, start_hour, duration_hours, status FROM reservations "
              "WHERE visit_date = ? AND status IN ('Confirmed', 'Seated')");
    q.addBindValue(date);
    if (q.exec()) {
        while (q.next()) {
            ActiveSlot s;
            s.tableId = q.value(0).toInt();
            s.startHour = q.value(1).toInt();
            s.durationHours = q.value(2).toInt();
            s.status = (q.value(3).toString() == "Seated") ? 2 : 1;
            slots.push_back(s);
        }
    }
    return slots;
}

int DatabaseManager::getTableOccupancyStatus(int tableId, const QString& date, int startHour, int durationHours) const {
    const int reqEnd = startHour + durationHours;
    int status = 0;
    for (const auto& s : loadActiveSlots(date)) {
        if (s.tableId != tableId) continue;
        if (startHour < s.startHour + s.durationHours && reqEnd > s.startHour) {
            status = std::max(status, s.status);
        }
    }
    return status;
}

QMap<int, int> DatabaseManager::getOccupancyMap(const QString& date, int startHour, int durationHours) const {
    QMap<int, int> result;
    const int reqEnd = startHour + durationHours;
    for (const auto& s : loadActiveSlots(date)) {
        if (startHour < s.startHour + s.durationHours && reqEnd > s.startHour) {
            result[s.tableId] = std::max(result.value(s.tableId, 0), s.status);
        }
    }
    return result;
}

std::vector<int> DatabaseManager::getTableDayStatuses(int tableId, const QString& date) const {
    std::vector<int> hours(static_cast<size_t>(CLOSE_HOUR - OPEN_HOUR), 0);
    for (const auto& s : loadActiveSlots(date)) {
        if (s.tableId != tableId) continue;
        const int from = std::max(s.startHour, OPEN_HOUR);
        const int to = std::min(s.startHour + s.durationHours, CLOSE_HOUR);
        for (int h = from; h < to; ++h) {
            int& cell = hours[static_cast<size_t>(h - OPEN_HOUR)];
            cell = std::max(cell, s.status);
        }
    }
    return hours;
}

std::vector<int> DatabaseManager::getBusyTableCountsByHour(const QString& date) const {
    std::vector<QSet<int>> busy(static_cast<size_t>(CLOSE_HOUR - OPEN_HOUR));
    for (const auto& s : loadActiveSlots(date)) {
        const int from = std::max(s.startHour, OPEN_HOUR);
        const int to = std::min(s.startHour + s.durationHours, CLOSE_HOUR);
        for (int h = from; h < to; ++h) {
            busy[static_cast<size_t>(h - OPEN_HOUR)].insert(s.tableId);
        }
    }
    std::vector<int> counts;
    counts.reserve(busy.size());
    for (const auto& set : busy) counts.push_back(static_cast<int>(set.size()));
    return counts;
}

bool DatabaseManager::isTableAvailable(int tableId, const QString& date, int startHour, int durationHours, int ignoreReservationId) const {
    if (durationHours <= 0 || durationHours > MAX_DURATION_HOURS) return false;
    if (startHour < OPEN_HOUR || startHour + durationHours > CLOSE_HOUR) return false;
    const int reqEnd = startHour + durationHours;
    QSqlQuery q(db_);
    q.prepare("SELECT id, start_hour, duration_hours FROM reservations "
              "WHERE table_id = ? AND visit_date = ? AND status IN ('Confirmed', 'Seated')");
    q.addBindValue(tableId);
    q.addBindValue(date);
    if (!q.exec()) return false;   // при ошибке БД считаем стол недоступным, а не свободным
    while (q.next()) {
        const int rId = q.value(0).toInt();
        if (rId == ignoreReservationId) continue;
        const int sHour = q.value(1).toInt();
        const int dur = q.value(2).toInt();
        if (startHour < (sHour + dur) && reqEnd > sHour) {
            return false;
        }
    }
    return true;
}

int DatabaseManager::getFreeHoursFrom(int tableId, const QString& date, int startHour) const {
    int freeHrs = 0;
    for (int h = std::max(startHour, OPEN_HOUR); h < CLOSE_HOUR; ++h) {
        if (isTableAvailable(tableId, date, h, 1)) ++freeHrs;
        else break;
    }
    return freeHrs;
}

// ---------------------------------------------------------------- Меню

std::vector<DishInfo> DatabaseManager::getAllDishes() const {
    std::vector<DishInfo> list;
    QSqlQuery q(db_);
    q.exec(QString("SELECT %1 FROM dishes ORDER BY id ASC").arg(kDishColumns));
    while (q.next()) {
        list.push_back(dishFromQuery(q));
    }
    return list;
}

DishInfo DatabaseManager::getDishById(int id) const {
    QSqlQuery q(db_);
    q.prepare(QString("SELECT %1 FROM dishes WHERE id = ?").arg(kDishColumns));
    q.addBindValue(id);
    if (q.exec() && q.next()) {
        return dishFromQuery(q);
    }
    return DishInfo{};
}

std::vector<DishInfo> DatabaseManager::searchDishes(const QString& category,
                                                    const QString& tag,
                                                    const QString& keyword,
                                                    double maxPrice) const {
    std::vector<DishInfo> filtered;
    const QString kw = keyword.trimmed();
    for (const auto& d : getAllDishes()) {
        if (!category.isEmpty() && category != "Все категории" && d.category != category) continue;
        if (!tag.isEmpty() && tag != "Все теги" && !d.tags.contains(tag)) continue;
        if (maxPrice > 0.0 && d.price > maxPrice) continue;
        if (!kw.isEmpty()) {
            const bool match = d.name.contains(kw, Qt::CaseInsensitive) ||
                               d.description.contains(kw, Qt::CaseInsensitive);
            if (!match) continue;
        }
        filtered.push_back(d);
    }
    return filtered;
}

bool DatabaseManager::addDish(const DishInfo& dish, QString* errorOut) {
    auto fail = [errorOut](const QString& msg) {
        if (errorOut) *errorOut = msg;
        return false;
    };

    const QString name = dish.name.simplified();
    if (name.size() < 2 || name.size() > 100) return fail("Название блюда должно содержать от 2 до 100 символов.");
    static const QStringList kCategories = {"Закуски", "Супы", "Горячее", "Десерты", "Напитки"};
    if (!kCategories.contains(dish.category)) return fail("Неизвестная категория блюда.");
    if (!(dish.price > 0.0) || dish.price > 100000.0) return fail("Цена должна быть больше 0 и не превышать 100 000 руб.");
    if (dish.weightGrams < 1 || dish.weightGrams > 5000) return fail("Вес блюда должен быть от 1 до 5000 г.");
    if (dish.calories < 0 || dish.calories > 5000) return fail("Калорийность должна быть от 0 до 5000 ккал.");
    if (dish.prepTimeMin < 1 || dish.prepTimeMin > 240) return fail("Время подачи должно быть от 1 до 240 минут.");
    if (dish.description.size() > MAX_COMMENT_LENGTH) return fail("Описание слишком длинное.");

    for (const auto& existing : getAllDishes()) {
        if (QString::compare(existing.name, name, Qt::CaseInsensitive) == 0) {
            return fail("Блюдо с таким названием уже есть в меню.");
        }
    }

    // Теги и аллергены не должны ломать формат «значения через запятую»
    QStringList tags;
    for (const QString& t : dish.tags) {
        const QString clean = t.trimmed().remove(',');
        if (!clean.isEmpty() && !tags.contains(clean)) tags << clean;
    }
    QStringList allergens;
    const QStringList vocabulary = allergenVocabulary();
    for (const QString& a : dish.allergens) {
        if (vocabulary.contains(a) && !allergens.contains(a)) allergens << a;
    }

    QSqlQuery q(db_);
    q.prepare(QString("INSERT INTO dishes (%1) VALUES (NULL,?,?,?,?,?,?,?,?,?)").arg(kDishColumns));
    q.addBindValue(name);
    q.addBindValue(dish.category);
    q.addBindValue(dish.price);
    q.addBindValue(dish.weightGrams);
    q.addBindValue(dish.calories);
    q.addBindValue(dish.prepTimeMin);
    q.addBindValue(tags.join(","));
    q.addBindValue(dish.description.trimmed());
    q.addBindValue(allergens.join(","));
    if (!q.exec()) return fail(describeDbError(q.lastError().text()));
    return true;
}

double DatabaseManager::calculatePreorderTotal(const std::vector<PreorderItem>& items, bool isOnline) const {
    double sum = 0.0;
    for (const auto& item : items) {
        sum += item.unitPrice * item.quantity;
    }
    if (isOnline) sum *= 0.95; // скидка 5% при онлайн-предзаказе
    return std::round(sum * 100.0) / 100.0;
}

std::vector<PreorderItem> DatabaseManager::loadPreorderForReservation(int reservationId) const {
    std::vector<PreorderItem> items;
    QSqlQuery q(db_);
    q.prepare("SELECT p.dish_id, d.name, d.price, p.quantity "
              "FROM preorder_items p JOIN dishes d ON p.dish_id = d.id "
              "WHERE p.reservation_id = ?");
    q.addBindValue(reservationId);
    if (q.exec()) {
        while (q.next()) {
            PreorderItem item;
            item.dishId = q.value(0).toInt();
            item.dishName = q.value(1).toString();
            item.unitPrice = q.value(2).toDouble();
            item.quantity = q.value(3).toInt();
            items.push_back(item);
        }
    }
    return items;
}

// ---------------------------------------------------------------- Бронирования

ReservationInfo DatabaseManager::readReservationRow(const QSqlQuery& q) const {
    ReservationInfo r;
    r.id = q.value(0).toInt();
    r.bookingCode = q.value(1).toString();
    r.guestName = q.value(2).toString();
    r.guestPhone = q.value(3).toString();
    r.visitDate = q.value(4).toString();
    r.startHour = q.value(5).toInt();
    r.durationHours = q.value(6).toInt();
    r.partySize = q.value(7).toInt();
    r.tableId = q.value(8).toInt();
    r.bookingType = q.value(9).toString();
    r.status = q.value(10).toString();
    r.occasion = q.value(11).toString();
    r.comment = q.value(12).toString();
    r.depositPaid = q.value(13).toDouble();
    r.preorder = loadPreorderForReservation(r.id);
    return r;
}

std::vector<ReservationInfo> DatabaseManager::getReservationsByDate(const QString& date) const {
    std::vector<ReservationInfo> list;
    QSqlQuery q(db_);
    q.prepare(QString("SELECT %1 FROM reservations WHERE visit_date = ? ORDER BY start_hour ASC, id ASC")
              .arg(kReservationColumns));
    q.addBindValue(date);
    if (q.exec()) {
        while (q.next()) {
            list.push_back(readReservationRow(q));
        }
    }
    return list;
}

std::vector<ReservationInfo> DatabaseManager::getAllReservations() const {
    std::vector<ReservationInfo> list;
    QSqlQuery q(db_);
    q.exec(QString("SELECT %1 FROM reservations ORDER BY visit_date DESC, start_hour ASC").arg(kReservationColumns));
    while (q.next()) {
        list.push_back(readReservationRow(q));
    }
    return list;
}

ReservationInfo DatabaseManager::getReservationById(int id) const {
    QSqlQuery q(db_);
    q.prepare(QString("SELECT %1 FROM reservations WHERE id = ?").arg(kReservationColumns));
    q.addBindValue(id);
    if (q.exec() && q.next()) {
        return readReservationRow(q);
    }
    return ReservationInfo{};
}

ReservationResult DatabaseManager::createReservation(const ReservationInfo& res) {
    ReservationResult out;
    auto fail = [&out](ReservationError error, const QString& message) {
        out.ok = false;
        out.id = 0;
        out.bookingCode.clear();
        out.error = error;
        out.message = message;
        return out;
    };

    // --- 1. Проверка входных данных (до обращения к БД)
    const QString name = res.guestName.simplified();
    if (!isValidGuestName(name)) {
        return fail(ReservationError::InvalidGuestName, "Укажите имя гостя (2–80 символов, должно содержать буквы).");
    }
    const QString phone = res.guestPhone.trimmed();
    if (!isValidPhone(phone)) {
        return fail(ReservationError::InvalidPhone, "Укажите корректный телефон: от 7 до 15 цифр (допустимы +, пробелы, скобки и дефисы).");
    }
    if (res.bookingType != "Online" && res.bookingType != "WalkIn") {
        return fail(ReservationError::InvalidField, "Неизвестный тип записи.");
    }
    if (!isValidDateString(res.visitDate)) {
        return fail(ReservationError::InvalidDate, "Некорректная дата визита.");
    }
    if (res.partySize < 1) {
        return fail(ReservationError::InvalidPartySize, "Количество гостей должно быть не меньше 1.");
    }
    if (res.durationHours < 1 || res.durationHours > MAX_DURATION_HOURS ||
        res.startHour < OPEN_HOUR || res.startHour + res.durationHours > CLOSE_HOUR) {
        return fail(ReservationError::InvalidTime,
                    QString("Бронь возможна только в рабочие часы (%1:00–%2:00), длительность 1–%3 ч. "
                            "Визит должен закончиться до закрытия.")
                    .arg(OPEN_HOUR).arg(CLOSE_HOUR).arg(MAX_DURATION_HOURS));
    }
    if (res.comment.size() > MAX_COMMENT_LENGTH) {
        return fail(ReservationError::InvalidField,
                    QString("Комментарий слишком длинный (максимум %1 символов).").arg(MAX_COMMENT_LENGTH));
    }

    const QDate visitDate = QDate::fromString(res.visitDate, "yyyy-MM-dd");
    const QDate today = QDate::currentDate();
    if (visitDate < today) {
        return fail(ReservationError::PastDateTime, "Нельзя оформить запись на прошедшую дату.");
    }
    if (res.bookingType == "WalkIn") {
        if (visitDate != today) {
            return fail(ReservationError::WalkInNotToday, "Посадка гостей «с улицы» возможна только на сегодняшнюю дату.");
        }
    } else if (visitDate == today && res.startHour < QTime::currentTime().hour()) {
        return fail(ReservationError::PastDateTime, "Выбранное время начала уже прошло.");
    }

    const QString occasion = res.occasion.trimmed().isEmpty() ? QString("Обычный визит") : res.occasion.trimmed();

    const TableInfo tbl = getTableById(res.tableId);
    if (tbl.id == 0) {
        return fail(ReservationError::TableNotFound, "Выбранный столик не найден.");
    }
    if (tbl.capacity < res.partySize) {
        return fail(ReservationError::CapacityExceeded,
                    QString("Стол #%1 [%2] рассчитан максимум на %3 чел., а гостей — %4.")
                    .arg(tbl.id).arg(tbl.code).arg(tbl.capacity).arg(res.partySize));
    }
    if (res.bookingType == "Online" && tbl.minDeposit > 0.0 && res.depositPaid + 0.005 < tbl.minDeposit) {
        return fail(ReservationError::DepositRequired,
                    QString("Для стола #%1 [%2] требуется депозит не менее %3 руб.")
                    .arg(tbl.id).arg(tbl.code).arg(tbl.minDeposit, 0, 'f', 0));
    }

    // Предзаказ: проверяем блюда и количество, одинаковые позиции объединяем
    std::vector<PreorderItem> items;
    for (const auto& it : res.preorder) {
        const DishInfo dish = getDishById(it.dishId);
        if (dish.id == 0 || it.quantity < 1) {
            return fail(ReservationError::InvalidPreorder, "В предзаказе есть несуществующее блюдо или неверное количество.");
        }
        auto found = std::find_if(items.begin(), items.end(),
                                  [&it](const PreorderItem& x) { return x.dishId == it.dishId; });
        if (found != items.end()) {
            found->quantity += it.quantity;
        } else {
            PreorderItem item;
            item.dishId = dish.id;
            item.dishName = dish.name;
            item.unitPrice = dish.price;
            item.quantity = it.quantity;
            items.push_back(item);
        }
    }
    for (const auto& it : items) {
        if (it.quantity > 50) {
            return fail(ReservationError::InvalidPreorder, "Количество одного блюда в предзаказе не может превышать 50.");
        }
    }

    // --- 2. Транзакция: проверка доступности и вставка выполняются под одной блокировкой записи
    QSqlQuery begin(db_);
    if (!begin.exec("BEGIN IMMEDIATE")) {
        return fail(ReservationError::DatabaseError, describeDbError(begin.lastError().text()));
    }
    auto rollback = [this]() {
        QSqlQuery rb(db_);
        rb.exec("ROLLBACK");
    };

    if (!isTableAvailable(res.tableId, res.visitDate, res.startHour, res.durationHours)) {
        rollback();
        return fail(ReservationError::SlotTaken,
                    QString("Стол #%1 [%2] уже занят на %3:00–%4:00. Выберите другой стол или время, "
                            "либо запишите гостя в лист ожидания.")
                    .arg(tbl.id).arg(tbl.code).arg(res.startHour).arg(res.startHour + res.durationHours));
    }

    int nextId = 1001;
    {
        QSqlQuery maxQ(db_);
        if (maxQ.exec("SELECT MAX(id) FROM reservations") && maxQ.next()) {
            nextId = std::max(1001, maxQ.value(0).toInt() + 1);
        }
    }
    const QString bookingCode = QString("BK-%1").arg(nextId);

    QSqlQuery ins(db_);
    ins.prepare(QString("INSERT INTO reservations (%1) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)").arg(kReservationColumns));
    ins.addBindValue(nextId);
    ins.addBindValue(bookingCode);
    ins.addBindValue(name);
    ins.addBindValue(phone);
    ins.addBindValue(res.visitDate);
    ins.addBindValue(res.startHour);
    ins.addBindValue(res.durationHours);
    ins.addBindValue(res.partySize);
    ins.addBindValue(res.tableId);
    ins.addBindValue(res.bookingType);
    ins.addBindValue(res.bookingType == "WalkIn" ? "Seated" : "Confirmed");
    ins.addBindValue(occasion);
    ins.addBindValue(res.comment.trimmed());
    ins.addBindValue(res.bookingType == "Online" ? std::max(0.0, res.depositPaid) : 0.0);

    if (!ins.exec()) {
        const QString dbText = ins.lastError().text();
        rollback();
        const ReservationError code = dbText.contains("table_overlap") ? ReservationError::SlotTaken
                                    : dbText.contains("party_exceeds_capacity") ? ReservationError::CapacityExceeded
                                    : ReservationError::DatabaseError;
        return fail(code, describeDbError(dbText));
    }

    for (const auto& item : items) {
        QSqlQuery pIns(db_);
        pIns.prepare("INSERT INTO preorder_items (reservation_id, dish_id, quantity) VALUES (?,?,?)");
        pIns.addBindValue(nextId);
        pIns.addBindValue(item.dishId);
        pIns.addBindValue(item.quantity);
        if (!pIns.exec()) {
            const QString dbText = pIns.lastError().text();
            rollback();   // бронь без предзаказа не сохраняем молча — откатываем всё
            return fail(ReservationError::InvalidPreorder, QString("Не удалось сохранить предзаказ: ") + describeDbError(dbText));
        }
    }

    QSqlQuery commit(db_);
    if (!commit.exec("COMMIT")) {
        const QString dbText = commit.lastError().text();
        rollback();
        return fail(ReservationError::DatabaseError, describeDbError(dbText));
    }

    out.ok = true;
    out.id = nextId;
    out.bookingCode = bookingCode;
    out.error = ReservationError::None;
    out.message.clear();
    return out;
}

bool DatabaseManager::updateReservationStatus(int reservationId, const QString& newStatus, QString* errorOut) {
    auto fail = [errorOut](const QString& msg) {
        if (errorOut) *errorOut = msg;
        return false;
    };

    QString current;
    QString date;
    {
        QSqlQuery sel(db_);
        sel.prepare("SELECT status, visit_date FROM reservations WHERE id = ?");
        sel.addBindValue(reservationId);
        if (!sel.exec() || !sel.next()) return fail("Бронь не найдена.");
        current = sel.value(0).toString();
        date = sel.value(1).toString();
    }

    const bool allowed =
        (current == "Confirmed" && (newStatus == "Seated" || newStatus == "Cancelled")) ||
        (current == "Seated" && newStatus == "Completed");
    if (!allowed) {
        return fail(QString("Недопустимое действие: бронь в статусе «%1» нельзя перевести в статус «%2».")
                    .arg(reservationStatusLabel(current), reservationStatusLabel(newStatus)));
    }
    if (newStatus == "Seated" && date != todayString()) {
        return fail("Посадить гостей за стол можно только в день визита.");
    }

    QSqlQuery q(db_);
    // Условие по текущему статусу защищает от «двойного клика» и от изменения брони из другого окна
    q.prepare("UPDATE reservations SET status = ? WHERE id = ? AND status = ?");
    q.addBindValue(newStatus);
    q.addBindValue(reservationId);
    q.addBindValue(current);
    if (!q.exec()) return fail(describeDbError(q.lastError().text()));
    if (q.numRowsAffected() <= 0) return fail("Статус брони уже был изменён. Обновите данные.");
    return true;
}

bool DatabaseManager::updateReservationComment(int reservationId, const QString& newComment) {
    if (newComment.size() > MAX_COMMENT_LENGTH) return false;
    QSqlQuery q(db_);
    q.prepare("UPDATE reservations SET comment = ? WHERE id = ?");
    q.addBindValue(newComment.trimmed());
    q.addBindValue(reservationId);
    return q.exec() && q.numRowsAffected() > 0;
}

std::vector<QString> DatabaseManager::findAllergenConflicts(const QString& comment,
                                                            const std::vector<PreorderItem>& preorder) const {
    std::vector<QString> warnings;
    const QString text = comment.toLower();
    if (!(text.contains(QString::fromUtf8("аллерг")) ||
          text.contains(QString::fromUtf8("непереносим")) ||
          text.contains(QString::fromUtf8("целиаки")))) {
        return warnings;
    }

    struct Keyword { const char* stem; const char* allergens; };
    static const Keyword kKeywords[] = {
        {"арахис", "орехи"}, {"орех", "орехи"}, {"фисташ", "орехи"}, {"кедров", "орехи"},
        {"миндал", "орехи"}, {"фундук", "орехи"},
        {"глютен", "глютен"}, {"целиаки", "глютен"}, {"пшениц", "глютен"},
        {"лактоз", "молоко"}, {"молок", "молоко"}, {"сливк", "молоко"},
        {"яйц", "яйца"}, {"рыб", "рыба"}, {"лосос", "рыба"},
        {"морепродукт", "ракообразные,моллюски"}, {"креветк", "ракообразные"}, {"краб", "ракообразные"},
        {"ракообраз", "ракообразные"}, {"моллюск", "моллюски"},
        {"соев", "соя"}, {"сою", "соя"}, {"кунжут", "кунжут"}
    };

    QSet<QString> guestAllergens;
    for (const auto& k : kKeywords) {
        if (text.contains(QString::fromUtf8(k.stem))) {
            const QStringList list = QString::fromUtf8(k.allergens).split(',', Qt::SkipEmptyParts);
            for (const QString& a : list) guestAllergens.insert(a);
        }
    }
    if (guestAllergens.isEmpty()) return warnings;

    for (const auto& item : preorder) {
        const DishInfo dish = getDishById(item.dishId);
        QStringList hits;
        for (const QString& a : dish.allergens) {
            if (guestAllergens.contains(a)) hits << a;
        }
        if (!hits.isEmpty()) {
            warnings.push_back(QString("«%1» — содержит: %2").arg(dish.name, hits.join(", ")));
        }
    }
    return warnings;
}

// ---------------------------------------------------------------- Лист ожидания

std::vector<WaitlistInfo> DatabaseManager::getWaitlist() const {
    std::vector<WaitlistInfo> list;
    QSqlQuery q(db_);
    q.exec("SELECT id, guest_name, guest_phone, visit_date, preferred_hour, party_size, comment "
           "FROM waitlist ORDER BY visit_date ASC, id ASC");
    while (q.next()) {
        WaitlistInfo w;
        w.id = q.value(0).toInt();
        w.guestName = q.value(1).toString();
        w.guestPhone = q.value(2).toString();
        w.visitDate = q.value(3).toString();
        w.preferredHour = q.value(4).toInt();
        w.partySize = q.value(5).toInt();
        w.comment = q.value(6).toString();
        list.push_back(w);
    }
    return list;
}

bool DatabaseManager::addToWaitlist(const WaitlistInfo& entry, QString* errorOut) {
    auto fail = [errorOut](const QString& msg) {
        if (errorOut) *errorOut = msg;
        return false;
    };

    const QString name = entry.guestName.simplified();
    if (!isValidGuestName(name)) return fail("Укажите имя гостя (2–80 символов, должно содержать буквы).");
    const QString phone = entry.guestPhone.trimmed();
    if (!isValidPhone(phone)) return fail("Укажите корректный телефон: от 7 до 15 цифр.");
    if (!isValidDateString(entry.visitDate)) return fail("Некорректная дата.");
    if (QDate::fromString(entry.visitDate, "yyyy-MM-dd") < QDate::currentDate()) {
        return fail("Нельзя записать в лист ожидания на прошедшую дату.");
    }
    if (entry.preferredHour < OPEN_HOUR || entry.preferredHour >= CLOSE_HOUR) {
        return fail(QString("Желаемое время должно быть в пределах рабочих часов (%1:00–%2:00).").arg(OPEN_HOUR).arg(CLOSE_HOUR));
    }
    if (entry.partySize < 1) return fail("Количество гостей должно быть не меньше 1.");
    if (entry.comment.size() > MAX_COMMENT_LENGTH) return fail("Комментарий слишком длинный.");

    QSqlQuery q(db_);
    q.prepare("INSERT INTO waitlist (guest_name, guest_phone, visit_date, preferred_hour, party_size, comment) "
              "VALUES (?,?,?,?,?,?)");
    q.addBindValue(name);
    q.addBindValue(phone);
    q.addBindValue(entry.visitDate);
    q.addBindValue(entry.preferredHour);
    q.addBindValue(entry.partySize);
    q.addBindValue(entry.comment.trimmed());
    if (!q.exec()) return fail(describeDbError(q.lastError().text()));
    return true;
}

bool DatabaseManager::removeFromWaitlist(int waitlistId) {
    QSqlQuery q(db_);
    q.prepare("DELETE FROM waitlist WHERE id = ?");
    q.addBindValue(waitlistId);
    return q.exec() && q.numRowsAffected() > 0;
}

// ---------------------------------------------------------------- Отзывы

std::vector<ReviewInfo> DatabaseManager::getAllReviews() const {
    std::vector<ReviewInfo> list;
    QSqlQuery q(db_);
    q.exec("SELECT id, author_name, review_date, rating, table_id, dish_id, comment, manager_reply "
           "FROM reviews ORDER BY id DESC");
    while (q.next()) {
        ReviewInfo r;
        r.id = q.value(0).toInt();
        r.authorName = q.value(1).toString();
        r.reviewDate = q.value(2).toString();
        r.rating = q.value(3).toInt();
        r.tableId = q.value(4).toInt();
        r.dishId = q.value(5).toInt();
        r.comment = q.value(6).toString();
        r.managerReply = q.value(7).toString();
        list.push_back(r);
    }
    return list;
}

bool DatabaseManager::addReview(const ReviewInfo& review, QString* errorOut) {
    auto fail = [errorOut](const QString& msg) {
        if (errorOut) *errorOut = msg;
        return false;
    };

    if (review.rating < 1 || review.rating > 5) return fail("Оценка должна быть от 1 до 5.");
    const QString comment = review.comment.trimmed();
    if (comment.size() < 3) return fail("Текст отзыва слишком короткий.");
    if (comment.size() > 1000) return fail("Текст отзыва слишком длинный (максимум 1000 символов).");
    const QString author = review.authorName.simplified();
    if (author.size() > 60) return fail("Имя слишком длинное (максимум 60 символов).");
    if (review.tableId < 0 || (review.tableId > 0 && getTableById(review.tableId).id == 0)) {
        return fail("Выбранный столик не найден.");
    }
    if (review.dishId < 0 || (review.dishId > 0 && getDishById(review.dishId).id == 0)) {
        return fail("Выбранное блюдо не найдено.");
    }

    QSqlQuery q(db_);
    q.prepare("INSERT INTO reviews (author_name, review_date, rating, table_id, dish_id, comment, manager_reply) "
              "VALUES (?,?,?,?,?,?,?)");
    q.addBindValue(author.isEmpty() ? QString("Гость") : author);
    q.addBindValue(review.reviewDate.isEmpty() ? todayString() : review.reviewDate);
    q.addBindValue(review.rating);
    q.addBindValue(review.tableId);
    q.addBindValue(review.dishId);
    q.addBindValue(comment);
    q.addBindValue(review.managerReply);
    if (!q.exec()) return fail(describeDbError(q.lastError().text()));
    return true;
}

bool DatabaseManager::addManagerReply(int reviewId, const QString& reply) {
    const QString text = reply.trimmed();
    if (text.isEmpty() || text.size() > 1000) return false;
    QSqlQuery q(db_);
    q.prepare("UPDATE reviews SET manager_reply = ? WHERE id = ?");
    q.addBindValue(text);
    q.addBindValue(reviewId);
    return q.exec() && q.numRowsAffected() > 0;
}

double DatabaseManager::getAverageTableRating(int tableId) const {
    QSqlQuery q(db_);
    q.prepare("SELECT AVG(rating) FROM reviews WHERE table_id = ?");
    q.addBindValue(tableId);
    if (q.exec() && q.next() && !q.value(0).isNull()) {
        return q.value(0).toDouble();
    }
    return 0.0;
}

QMap<int, double> DatabaseManager::getAverageTableRatings() const {
    QMap<int, double> ratings;
    QSqlQuery q(db_);
    if (q.exec("SELECT table_id, AVG(rating) FROM reviews WHERE table_id > 0 GROUP BY table_id")) {
        while (q.next()) {
            ratings[q.value(0).toInt()] = q.value(1).toDouble();
        }
    }
    return ratings;
}

double DatabaseManager::getAverageDishRating(int dishId) const {
    QSqlQuery q(db_);
    q.prepare("SELECT AVG(rating) FROM reviews WHERE dish_id = ?");
    q.addBindValue(dishId);
    if (q.exec() && q.next() && !q.value(0).isNull()) {
        return q.value(0).toDouble();
    }
    return 0.0;
}

double DatabaseManager::getOverallRating() const {
    QSqlQuery q(db_);
    if (q.exec("SELECT AVG(rating) FROM reviews") && q.next() && !q.value(0).isNull()) {
        return q.value(0).toDouble();
    }
    return 0.0;
}

// ---------------------------------------------------------------- Рекомендации

std::vector<TableRecommendation> DatabaseManager::recommendTables(const TablePreference& pref) const {
    std::vector<TableRecommendation> recs;
    if (pref.durationHours < 1 || pref.durationHours > MAX_DURATION_HOURS ||
        pref.startHour < OPEN_HOUR || pref.startHour + pref.durationHours > CLOSE_HOUR) {
        return recs;   // вне рабочих часов свободных столов нет
    }

    const QMap<int, int> occupancy = getOccupancyMap(pref.visitDate, pref.startHour, pref.durationHours);
    const QMap<int, double> ratings = getAverageTableRatings();

    for (const auto& t : getAllTables()) {
        if (t.capacity < pref.partySize) continue;
        if (occupancy.value(t.id, 0) != 0) continue;

        TableRecommendation rec;
        rec.table = t;
        rec.avgRating = ratings.value(t.id, 0.0);
        int score = 50;

        const int extra = t.capacity - pref.partySize;
        if (extra == 0) {
            score += 18;
            rec.reasons << QString("Точное совпадение по числу мест (%1 чел.)").arg(t.capacity);
        } else if (extra <= 2) {
            score += 10;
            rec.reasons << QString("Комфортный запас мест (%1 чел.)").arg(t.capacity);
        } else {
            score -= std::min(15, extra * 3);
        }

        if (pref.occasion.contains("Свидание", Qt::CaseInsensitive)) {
            if (t.nearWindow) { score += 15; rec.reasons << "Панорамный вид из окна для романтики"; }
            if (t.isQuiet)    { score += 10; rec.reasons << "Тихое уединенное расположение"; }
        } else if (pref.occasion.contains("Деловая", Qt::CaseInsensitive)) {
            if (t.isQuiet)    { score += 16; rec.reasons << "Тихая зона для деловых переговоров"; }
            if (t.hasOutlet)  { score += 12; rec.reasons << "Розетка 220В/USB для ноутбука"; }
            if (t.zone.contains("VIP")) { score += 10; rec.reasons << "Статусная обстановка VIP-зала"; }
        } else if (pref.occasion.contains("День рождения", Qt::CaseInsensitive)) {
            if (t.hasSofa)    { score += 14; rec.reasons << "Просторные диваны для компании"; }
            if (t.zone.contains("VIP") || t.zone.contains("терраса")) { score += 12; rec.reasons << "Отличная зона для праздника"; }
        } else if (pref.occasion.contains("Семейный", Qt::CaseInsensitive)) {
            if (t.hasSofa)    { score += 14; rec.reasons << "Мягкие диваны для отдыха с семьей"; }
            if (t.isQuiet)    { score += 8;  rec.reasons << "Спокойная семейная обстановка"; }
        }

        if (pref.preferWindow && t.nearWindow) { score += 10; rec.reasons << "Пожелание: у панорамного окна"; }
        if (pref.preferQuiet && t.isQuiet)     { score += 10; rec.reasons << "Пожелание: тихая зона"; }
        if (pref.needOutlet && t.hasOutlet)    { score += 10; rec.reasons << "Пожелание: розетка 220В"; }
        if (pref.preferSofa && t.hasSofa)      { score += 10; rec.reasons << "Пожелание: мягкий диван"; }

        if (t.minDeposit > 0.0) {
            rec.reasons << QString("Внимание: для этого стола нужен депозит %1 руб.").arg(t.minDeposit, 0, 'f', 0);
        }

        if (rec.avgRating >= 4.5) {
            score += 8;
            rec.reasons << QString("Высокий рейтинг гостей (★ %1/5.0)").arg(rec.avgRating, 0, 'f', 1);
        }

        rec.matchScore = std::clamp(score, 10, 99);
        if (rec.reasons.isEmpty()) rec.reasons << "Свободный подходящий столик";
        recs.push_back(rec);
    }

    std::stable_sort(recs.begin(), recs.end(), [](const TableRecommendation& a, const TableRecommendation& b) {
        if (a.matchScore != b.matchScore) return a.matchScore > b.matchScore;
        return a.avgRating > b.avgRating;
    });
    return recs;
}

std::vector<MenuSetRecommendation> DatabaseManager::recommendMenuSets(const MenuPreference& pref) const {
    std::vector<MenuSetRecommendation> sets;
    const auto allDishes = getAllDishes();

    // Диетические ограничения («вегетарианское», «без глютена») действуют на все блюда сета и не
    // «разбавляются» блюдами без нужного тега. Остальные теги — это предпочтение для основного блюда.
    const bool hasTag = !pref.dietaryTag.isEmpty() && pref.dietaryTag != "Все";
    const bool strictDiet = hasTag && (pref.dietaryTag == "вегетарианское" || pref.dietaryTag == "без глютена");

    auto allowedAnywhere = [&](const DishInfo& d) {
        if (pref.avoidSpicy && d.tags.contains("острое")) return false;
        if (strictDiet && !d.tags.contains(pref.dietaryTag)) return false;
        return true;
    };
    auto allowedMain = [&](const DishInfo& d) {
        return allowedAnywhere(d) && (!hasTag || d.tags.contains(pref.dietaryTag));
    };

    struct Candidate {
        DishInfo dish;
        double rating;
    };
    std::vector<Candidate> starters, mains, desserts, drinks;
    for (const auto& d : allDishes) {
        const Candidate c{d, getAverageDishRating(d.id)};
        if (d.category == "Закуски" || d.category == "Супы") {
            if (allowedAnywhere(d)) starters.push_back(c);
        } else if (d.category == "Горячее") {
            if (allowedMain(d)) mains.push_back(c);
        } else if (d.category == "Десерты") {
            if (allowedAnywhere(d)) desserts.push_back(c);
        } else if (d.category == "Напитки") {
            if (allowedAnywhere(d)) drinks.push_back(c);
        }
    }

    auto byRating = [](const Candidate& a, const Candidate& b) {
        if (a.rating != b.rating) return a.rating > b.rating;
        return a.dish.id < b.dish.id;
    };
    std::stable_sort(starters.begin(), starters.end(), byRating);
    std::stable_sort(mains.begin(), mains.end(), byRating);
    std::stable_sort(desserts.begin(), desserts.end(), byRating);
    std::stable_sort(drinks.begin(), drinks.end(), byRating);

    // Берёт из списка первое подходящее по цене/калориям блюдо, начиная со смещения (чтобы сеты отличались)
    auto pickFrom = [](const std::vector<Candidate>& pool, size_t offset, double priceLimit, int caloriesLeft) -> const DishInfo* {
        for (size_t i = 0; i < pool.size(); ++i) {
            const DishInfo& d = pool[(offset + i) % pool.size()].dish;
            if (d.price > priceLimit) continue;
            if (caloriesLeft >= 0 && d.calories > caloriesLeft) continue;
            return &d;
        }
        return nullptr;
    };

    for (size_t i = 0; i < mains.size() && sets.size() < 3; ++i) {
        const DishInfo& mainDish = mains[i].dish;
        if (mainDish.price > pref.maxBudget) continue;
        if (pref.maxCalories > 0 && mainDish.calories > pref.maxCalories) continue;

        MenuSetRecommendation s;
        s.dishes.push_back(mainDish);
        s.totalPrice = mainDish.price;
        s.totalCalories = mainDish.calories;
        const size_t variant = sets.size();

        auto caloriesLeft = [&]() { return pref.maxCalories > 0 ? pref.maxCalories - s.totalCalories : -1; };

        if (const DishInfo* st = pickFrom(starters, variant, pref.maxBudget * 0.85 - s.totalPrice, caloriesLeft())) {
            s.dishes.insert(s.dishes.begin(), *st);
            s.totalPrice += st->price;
            s.totalCalories += st->calories;
        }
        if (const DishInfo* dr = pickFrom(drinks, variant, pref.maxBudget - s.totalPrice, caloriesLeft())) {
            s.dishes.push_back(*dr);
            s.totalPrice += dr->price;
            s.totalCalories += dr->calories;
        }
        if (const DishInfo* ds = pickFrom(desserts, variant, pref.maxBudget - s.totalPrice, caloriesLeft())) {
            s.dishes.push_back(*ds);
            s.totalPrice += ds->price;
            s.totalCalories += ds->calories;
        }

        s.title = QString("Сет №%1").arg(variant + 1);
        QString rating = mains[i].rating > 0.0 ? QString(" (★ %1)").arg(mains[i].rating, 0, 'f', 1) : QString();
        s.explanation = QString("Основное блюдо: %1%2. Блюд в сете: %3, всего %4 ккал.")
                        .arg(mainDish.name, rating)
                        .arg(s.dishes.size())
                        .arg(s.totalCalories);
        sets.push_back(s);
    }
    return sets;
}

} // namespace Restaurant
