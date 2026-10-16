#include "DatabaseManager.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <algorithm>
#include <cmath>

namespace Restaurant {

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::openDatabase(const QString& dbFilePath) {
    QFileInfo fi(dbFilePath);
    if (fi.dir().exists() == false) {
        QDir().mkpath(fi.dir().absolutePath());
    }

    if (QSqlDatabase::contains("restaurant_sql_conn")) {
        db_ = QSqlDatabase::database("restaurant_sql_conn");
    } else {
        db_ = QSqlDatabase::addDatabase("QSQLITE", "restaurant_sql_conn");
    }
    db_.setDatabaseName(dbFilePath);

    if (!db_.open()) {
        qWarning() << "Не удалось открыть базу данных SQLite:" << db_.lastError().text();
        return false;
    }

    initializeSchemaAndSeedData();
    return true;
}

void DatabaseManager::closeDatabase() {
    if (db_.isOpen()) {
        db_.close();
    }
}

void DatabaseManager::initializeSchemaAndSeedData() {
    QSqlQuery q(db_);
    q.exec("PRAGMA foreign_keys = ON;");

    q.exec("CREATE TABLE IF NOT EXISTS tables ("
           "id INTEGER PRIMARY KEY, "
           "code TEXT NOT NULL UNIQUE, "
           "zone TEXT NOT NULL, "
           "capacity INTEGER NOT NULL, "
           "grid_x INTEGER NOT NULL, "
           "grid_y INTEGER NOT NULL, "
           "has_outlet INTEGER NOT NULL DEFAULT 0, "
           "is_quiet INTEGER NOT NULL DEFAULT 0, "
           "has_sofa INTEGER NOT NULL DEFAULT 0, "
           "near_window INTEGER NOT NULL DEFAULT 0, "
           "min_deposit REAL NOT NULL DEFAULT 0.0, "
           "description TEXT NOT NULL)");

    q.exec("CREATE TABLE IF NOT EXISTS dishes ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "name TEXT NOT NULL, "
           "category TEXT NOT NULL, "
           "price REAL NOT NULL, "
           "weight_grams INTEGER NOT NULL, "
           "calories INTEGER NOT NULL, "
           "prep_time_min INTEGER NOT NULL, "
           "tags TEXT NOT NULL, "
           "description TEXT NOT NULL)");

    q.exec("CREATE TABLE IF NOT EXISTS reservations ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "booking_code TEXT NOT NULL UNIQUE, "
           "guest_name TEXT NOT NULL, "
           "guest_phone TEXT NOT NULL, "
           "visit_date TEXT NOT NULL, "
           "start_hour INTEGER NOT NULL, "
           "duration_hours INTEGER NOT NULL, "
           "party_size INTEGER NOT NULL, "
           "table_id INTEGER NOT NULL, "
           "booking_type TEXT NOT NULL, "
           "status TEXT NOT NULL, "
           "occasion TEXT NOT NULL, "
           "comment TEXT NOT NULL DEFAULT '', "
           "FOREIGN KEY (table_id) REFERENCES tables(id))");

    q.exec("CREATE TABLE IF NOT EXISTS preorder_items ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "reservation_id INTEGER NOT NULL, "
           "dish_id INTEGER NOT NULL, "
           "quantity INTEGER NOT NULL DEFAULT 1, "
           "FOREIGN KEY (reservation_id) REFERENCES reservations(id) ON DELETE CASCADE, "
           "FOREIGN KEY (dish_id) REFERENCES dishes(id))");

    q.exec("CREATE TABLE IF NOT EXISTS reviews ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "author_name TEXT NOT NULL, "
           "review_date TEXT NOT NULL, "
           "rating INTEGER NOT NULL, "
           "table_id INTEGER NOT NULL DEFAULT 0, "
           "dish_id INTEGER NOT NULL DEFAULT 0, "
           "comment TEXT NOT NULL, "
           "manager_reply TEXT NOT NULL DEFAULT '')");

    q.exec("CREATE TABLE IF NOT EXISTS waitlist ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "guest_name TEXT NOT NULL, "
           "guest_phone TEXT NOT NULL, "
           "visit_date TEXT NOT NULL, "
           "preferred_hour INTEGER NOT NULL, "
           "party_size INTEGER NOT NULL, "
           "comment TEXT NOT NULL DEFAULT '')");

    // Проверяем, заполнены ли таблицы. Если база новая, заполняем стартовыми данными
    q.exec("SELECT COUNT(*) FROM tables");
    if (q.next() && q.value(0).toInt() == 0) {
        db_.transaction();

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
            ins.prepare("INSERT INTO tables VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");
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
            ins.exec();
        }

        const std::vector<DishInfo> defaultDishes = {
            {1,  "Тартар из лосося с манго и авокадо", "Закуски", 890.0,  190, 310, 12, {"морепродукты", "без глютена", "легкое", "хит шефа"}, "Охлажденный фарерский лосось, спелое манго, мусс из авокадо и соус понзу"},
            {2,  "Буррата с узбекскими томатами и песто", "Закуски", 850.0,  240, 390, 10, {"вегетарианское", "без глютена", "хит шефа"}, "Фермерская сливочная буррата, сладкие томаты, кедровый орех и базиликовый песто"},
            {3,  "Крем-суп из белых грибов с трюфельным маслом", "Супы", 640.0,  300, 340, 15, {"вегетарианское"}, "Ароматные белые грибы, сливки 22%, чипсы из пармезана и капля белого трюфеля"},
            {4,  "Том Ям с тигровыми креветками и кальмаром", "Супы", 920.0,  380, 410, 18, {"морепродукты", "острое", "без глютена"}, "Пряный бульон на кокосовом молоке с лемонграссом, галангалом, креветками и рисом жасмин"},
            {5,  "Стейк Рибай Prime с перечным демигласом", "Горячее", 2450.0, 320, 760, 25, {"без глютена", "хит шефа"}, "Мраморная говядина зернового откорма, запеченный чеснок, розмарин и соус из трех перцев"},
            {6,  "Филе палтуса на подушке из зеленой спаржи", "Горячее", 1490.0, 260, 420, 20, {"морепродукты", "без глютена", "легкое"}, "Нежное филе северного палтуса су-вид, молодая спаржа на гриле и шафрановый велюте"},
            {7,  "Ризотто с лесными лисичками и страчателлой", "Горячее", 980.0,  290, 540, 20, {"вегетарианское", "без глютена"}, "Рис карнароли, обжаренные лисички, сливочная страчателла и выдержанный пармезан"},
            {8,  "Утиная грудка Магре с грушей в пряном вине", "Горячее", 1350.0, 270, 590, 22, {"хит шефа"}, "Розовая утиная грудка с хрустящей корочкой, томленая груша и ягодный взвар"},
            {9,  "Паста Тальятелле с камчатским крабом", "Горячее", 1620.0, 280, 560, 18, {"морепродукты", "хит шефа"}, "Домашняя паста ручной работы, первая фаланга камчатского краба и соус биск"},
            {10, "Фисташковый тирамису с малиновым кули", "Десерты", 590.0,  160, 430, 10, {"вегетарианское", "хит шефа"}, "Воздушный крем маскарпоне с иранской фисташковой пастой, савоярди и свежая малина"},
            {11, "Сорбет Манго-Маракуйя с семенами чиа", "Десерты", 420.0,  140, 165, 7, {"вегетарианское", "без глютена", "легкое"}, "Освежающий натуральный сорбет без сахара со свежей мятой и чиа"},
            {12, "Авторский лимонад «Лаванда — Дикая Ежевика»", "Напитки", 380.0,  350, 110, 5, {"вегетарианское", "без глютена", "легкое"}, "Настой крымской лаванды, пюре лесной ежевики, сок лайма и родниковая вода"},
            {13, "Чайник согревающего чая «Облепиха — Имбирь — Груша»", "Напитки", 490.0,  600, 140, 8, {"вегетарианское", "без глютена", "хит шефа"}, "Таежная облепиха, слайсы медовой груши, свежий корень имбиря и корица"},
            {14, "Безалкогольный коктейль «Botanical Spritz»", "Напитки", 460.0,  280, 95,  6, {"вегетарианское", "без глютена", "легкое"}, "Безалкогольный аперитив на травах, грейпфрутовый кордиал и тоник"}
        };

        for (const auto& d : defaultDishes) {
            QSqlQuery ins(db_);
            ins.prepare("INSERT INTO dishes (id, name, category, price, weight_grams, calories, prep_time_min, tags, description) "
                        "VALUES (?,?,?,?,?,?,?,?,?)");
            ins.addBindValue(d.id);
            ins.addBindValue(d.name);
            ins.addBindValue(d.category);
            ins.addBindValue(d.price);
            ins.addBindValue(d.weightGrams);
            ins.addBindValue(d.calories);
            ins.addBindValue(d.prepTimeMin);
            ins.addBindValue(d.tags.join(","));
            ins.addBindValue(d.description);
            ins.exec();
        }

        q.exec("INSERT INTO reservations VALUES "
               "(1001, 'BK-1001', 'Алексей Смирнов', '+7 (916) 555-12-34', '2026-10-16', 19, 2, 2, 1, 'Online', 'Confirmed', 'Свидание', 'Годовщина знакомства, просьба поставить свечу и принести комплимент'),"
               "(1002, 'BK-1002', 'Екатерина Волкова', '+7 (926) 777-88-99', '2026-10-16', 18, 3, 5, 9, 'Online', 'Seated', 'Деловая встреча', 'Переговоры с партнерами, нужен тихий стол и 2 меню без цен'),"
               "(1003, 'BK-1003', 'Дмитрий Орлов (Walk-in)', '+7 (903) 111-22-33', '2026-10-16', 19, 2, 3, 5, 'WalkIn', 'Seated', 'Обычный визит', 'Гости зашли с улицы, нужен детский стульчик'),"
               "(1004, 'BK-1004', 'Мария Соколова', '+7 (999) 444-55-66', '2026-10-16', 20, 2, 4, 3, 'Online', 'Confirmed', 'День рождения', 'Аллергия на арахис у одного гостя, вынести десерт со свечой в 21:00')");

        q.exec("INSERT INTO preorder_items (reservation_id, dish_id, quantity) VALUES "
               "(1001, 1, 1), (1001, 6, 1), (1001, 8, 1), (1001, 10, 2),"
               "(1002, 2, 2), (1002, 5, 3), (1002, 13, 2),"
               "(1003, 7, 2), (1003, 12, 3),"
               "(1004, 9, 2), (1004, 10, 2), (1004, 14, 4)");

        q.exec("INSERT INTO reviews (id, author_name, review_date, rating, table_id, dish_id, comment, manager_reply) VALUES "
               "(1, 'Алина К.', '2026-10-12', 5, 1, 1, 'Потрясающий столик W1 прямо у окна! Тартар из лосося с манго просто тает во рту.', 'Благодарим за теплые слова, Алина! Будем рады видеть вас снова за вашим любимым столиком.'),"
               "(2, 'Игорь В.', '2026-10-13', 5, 9, 5, 'Проводили деловой ужин в VIP-зале у камина (стол V1). Тихо, солидно, стейк Рибай идеальной прожарки Medium.', 'Спасибо за высокую оценку кухни и сервиса, Игорь!'),"
               "(3, 'Светлана М.', '2026-10-14', 4, 3, 9, 'Паста с крабом восхитительная, на веранде очень уютно благодаря пледам. Хотелось бы чуть быстрее подачу напитков.', 'Светлана, спасибо за обратную связь! Мы усилили смену барменов в вечерние часы.'),"
               "(4, 'Максим П.', '2026-10-15', 5, 2, 6, 'Отмечали свидание за столиком W2. Палтус на спарже и фисташковый тирамису — 10 из 10!', 'Максим, рады, что ваш вечер прошел великолепно!')");

        db_.commit();
    }
}

std::vector<TableInfo> DatabaseManager::getAllTables() const {
    std::vector<TableInfo> list;
    QSqlQuery q(db_);
    q.exec("SELECT id, code, zone, capacity, grid_x, grid_y, has_outlet, is_quiet, has_sofa, near_window, min_deposit, description "
           "FROM tables ORDER BY id ASC");
    while (q.next()) {
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
        list.push_back(t);
    }
    return list;
}

TableInfo DatabaseManager::getTableById(int id) const {
    QSqlQuery q(db_);
    q.prepare("SELECT id, code, zone, capacity, grid_x, grid_y, has_outlet, is_quiet, has_sofa, near_window, min_deposit, description "
              "FROM tables WHERE id = ?");
    q.addBindValue(id);
    if (q.exec() && q.next()) {
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
    return TableInfo{};
}

int DatabaseManager::getTableOccupancyStatus(int tableId, const QString& date, int startHour, int durationHours) const {
    int reqEnd = startHour + durationHours;
    QSqlQuery q(db_);
    q.prepare("SELECT start_hour, duration_hours, status FROM reservations "
              "WHERE table_id = ? AND visit_date = ? AND status IN ('Confirmed', 'Seated')");
    q.addBindValue(tableId);
    q.addBindValue(date);
    int status = 0;
    if (q.exec()) {
        while (q.next()) {
            int sHour = q.value(0).toInt();
            int dur = q.value(1).toInt();
            QString st = q.value(2).toString();
            if (startHour < (sHour + dur) && reqEnd > sHour) {
                if (st == "Seated") return 2;
                status = 1;
            }
        }
    }
    return status;
}

bool DatabaseManager::isTableAvailable(int tableId, const QString& date, int startHour, int durationHours, int ignoreReservationId) const {
    if (startHour < OPEN_HOUR || startHour + durationHours > CLOSE_HOUR || durationHours <= 0) return false;
    int reqEnd = startHour + durationHours;
    QSqlQuery q(db_);
    q.prepare("SELECT id, start_hour, duration_hours FROM reservations "
              "WHERE table_id = ? AND visit_date = ? AND status IN ('Confirmed', 'Seated')");
    q.addBindValue(tableId);
    q.addBindValue(date);
    if (q.exec()) {
        while (q.next()) {
            int rId = q.value(0).toInt();
            if (rId == ignoreReservationId) continue;
            int sHour = q.value(1).toInt();
            int dur = q.value(2).toInt();
            if (startHour < (sHour + dur) && reqEnd > sHour) {
                return false;
            }
        }
    }
    return true;
}

int DatabaseManager::getFreeHoursFrom(int tableId, const QString& date, int startHour) const {
    int freeHrs = 0;
    for (int h = startHour; h < CLOSE_HOUR; ++h) {
        if (isTableAvailable(tableId, date, h, 1)) ++freeHrs;
        else break;
    }
    return freeHrs;
}

std::vector<DishInfo> DatabaseManager::getAllDishes() const {
    std::vector<DishInfo> list;
    QSqlQuery q(db_);
    q.exec("SELECT id, name, category, price, weight_grams, calories, prep_time_min, tags, description "
           "FROM dishes ORDER BY id ASC");
    while (q.next()) {
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
        list.push_back(d);
    }
    return list;
}

DishInfo DatabaseManager::getDishById(int id) const {
    QSqlQuery q(db_);
    q.prepare("SELECT id, name, category, price, weight_grams, calories, prep_time_min, tags, description "
              "FROM dishes WHERE id = ?");
    q.addBindValue(id);
    if (q.exec() && q.next()) {
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
        return d;
    }
    return DishInfo{};
}

std::vector<DishInfo> DatabaseManager::searchDishes(const QString& category,
                                                    const QString& tag,
                                                    const QString& keyword,
                                                    double maxPrice) const {
    std::vector<DishInfo> filtered;
    for (const auto& d : getAllDishes()) {
        if (!category.isEmpty() && category != "Все категории" && d.category != category) continue;
        if (!tag.isEmpty() && tag != "Все теги" && !d.tags.contains(tag)) continue;
        if (maxPrice > 0.0 && d.price > maxPrice) continue;
        if (!keyword.trimmed().isEmpty()) {
            QString kw = keyword.trimmed();
            bool match = d.name.contains(kw, Qt::CaseInsensitive) ||
                         d.description.contains(kw, Qt::CaseInsensitive);
            if (!match) continue;
        }
        filtered.push_back(d);
    }
    return filtered;
}

bool DatabaseManager::addDish(const DishInfo& dish) {
    QSqlQuery q(db_);
    q.prepare("INSERT INTO dishes (name, category, price, weight_grams, calories, prep_time_min, tags, description) "
              "VALUES (?,?,?,?,?,?,?,?)");
    q.addBindValue(dish.name);
    q.addBindValue(dish.category);
    q.addBindValue(dish.price);
    q.addBindValue(dish.weightGrams);
    q.addBindValue(dish.calories);
    q.addBindValue(dish.prepTimeMin);
    q.addBindValue(dish.tags.join(","));
    q.addBindValue(dish.description);
    return q.exec();
}

double DatabaseManager::calculatePreorderTotal(const std::vector<PreorderItem>& items, bool isOnline) const {
    double sum = 0.0;
    for (const auto& item : items) {
        sum += item.unitPrice * item.quantity;
    }
    if (isOnline) sum *= 0.95; // скидка 5% при онлайн-предзаказе
    return sum;
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

std::vector<ReservationInfo> DatabaseManager::getReservationsByDate(const QString& date) const {
    std::vector<ReservationInfo> list;
    QSqlQuery q(db_);
    q.prepare("SELECT id, booking_code, guest_name, guest_phone, visit_date, start_hour, duration_hours, "
              "party_size, table_id, booking_type, status, occasion, comment "
              "FROM reservations WHERE visit_date = ? ORDER BY start_hour ASC, id ASC");
    q.addBindValue(date);
    if (q.exec()) {
        while (q.next()) {
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
            r.preorder = loadPreorderForReservation(r.id);
            list.push_back(r);
        }
    }
    return list;
}

std::vector<ReservationInfo> DatabaseManager::getAllReservations() const {
    std::vector<ReservationInfo> list;
    QSqlQuery q(db_);
    q.exec("SELECT id, booking_code, guest_name, guest_phone, visit_date, start_hour, duration_hours, "
           "party_size, table_id, booking_type, status, occasion, comment "
           "FROM reservations ORDER BY visit_date DESC, start_hour ASC");
    while (q.next()) {
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
        r.preorder = loadPreorderForReservation(r.id);
        list.push_back(r);
    }
    return list;
}

ReservationInfo DatabaseManager::getReservationById(int id) const {
    QSqlQuery q(db_);
    q.prepare("SELECT id, booking_code, guest_name, guest_phone, visit_date, start_hour, duration_hours, "
              "party_size, table_id, booking_type, status, occasion, comment "
              "FROM reservations WHERE id = ?");
    q.addBindValue(id);
    if (q.exec() && q.next()) {
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
        r.preorder = loadPreorderForReservation(r.id);
        return r;
    }
    return ReservationInfo{};
}

int DatabaseManager::createReservation(const ReservationInfo& res, QString& outBookingCode) {
    TableInfo tbl = getTableById(res.tableId);
    if (tbl.id == 0 || tbl.capacity < res.partySize) return 0;
    if (!isTableAvailable(res.tableId, res.visitDate, res.startHour, res.durationHours)) return 0;

    db_.transaction();

    int nextId = 1001;
    QSqlQuery maxQ(db_);
    if (maxQ.exec("SELECT MAX(id) FROM reservations") && maxQ.next()) {
        nextId = std::max(1001, maxQ.value(0).toInt() + 1);
    }
    outBookingCode = QString("BK-%1").arg(nextId);

    QSqlQuery ins(db_);
    ins.prepare("INSERT INTO reservations (id, booking_code, guest_name, guest_phone, visit_date, "
                "start_hour, duration_hours, party_size, table_id, booking_type, status, occasion, comment) "
                "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)");
    ins.addBindValue(nextId);
    ins.addBindValue(outBookingCode);
    ins.addBindValue(res.guestName.trimmed().isEmpty() ? "Гость" : res.guestName.trimmed());
    ins.addBindValue(res.guestPhone.trimmed().isEmpty() ? "-" : res.guestPhone.trimmed());
    ins.addBindValue(res.visitDate);
    ins.addBindValue(res.startHour);
    ins.addBindValue(res.durationHours);
    ins.addBindValue(res.partySize);
    ins.addBindValue(res.tableId);
    ins.addBindValue(res.bookingType);
    ins.addBindValue(res.bookingType == "WalkIn" ? "Seated" : "Confirmed");
    ins.addBindValue(res.occasion);
    ins.addBindValue(res.comment);

    if (!ins.exec()) {
        db_.rollback();
        return 0;
    }

    for (const auto& item : res.preorder) {
        if (item.quantity <= 0) continue;
        QSqlQuery pIns(db_);
        pIns.prepare("INSERT INTO preorder_items (reservation_id, dish_id, quantity) VALUES (?,?,?)");
        pIns.addBindValue(nextId);
        pIns.addBindValue(item.dishId);
        pIns.addBindValue(item.quantity);
        pIns.exec();
    }

    db_.commit();
    return nextId;
}

bool DatabaseManager::updateReservationStatus(int reservationId, const QString& newStatus) {
    QSqlQuery q(db_);
    q.prepare("UPDATE reservations SET status = ? WHERE id = ?");
    q.addBindValue(newStatus);
    q.addBindValue(reservationId);
    return q.exec() && q.numRowsAffected() > 0;
}

bool DatabaseManager::updateReservationComment(int reservationId, const QString& newComment) {
    QSqlQuery q(db_);
    q.prepare("UPDATE reservations SET comment = ? WHERE id = ?");
    q.addBindValue(newComment);
    q.addBindValue(reservationId);
    return q.exec() && q.numRowsAffected() > 0;
}

std::vector<WaitlistInfo> DatabaseManager::getWaitlist() const {
    std::vector<WaitlistInfo> list;
    QSqlQuery q(db_);
    q.exec("SELECT id, guest_name, guest_phone, visit_date, preferred_hour, party_size, comment "
           "FROM waitlist ORDER BY id DESC");
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

bool DatabaseManager::addToWaitlist(const WaitlistInfo& entry) {
    QSqlQuery q(db_);
    q.prepare("INSERT INTO waitlist (guest_name, guest_phone, visit_date, preferred_hour, party_size, comment) "
              "VALUES (?,?,?,?,?,?)");
    q.addBindValue(entry.guestName);
    q.addBindValue(entry.guestPhone);
    q.addBindValue(entry.visitDate);
    q.addBindValue(entry.preferredHour);
    q.addBindValue(entry.partySize);
    q.addBindValue(entry.comment);
    return q.exec();
}

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

bool DatabaseManager::addReview(const ReviewInfo& review) {
    QSqlQuery q(db_);
    q.prepare("INSERT INTO reviews (author_name, review_date, rating, table_id, dish_id, comment, manager_reply) "
              "VALUES (?,?,?,?,?,?,?)");
    q.addBindValue(review.authorName.trimmed().isEmpty() ? "Гость" : review.authorName.trimmed());
    q.addBindValue(review.reviewDate);
    q.addBindValue(std::clamp(review.rating, 1, 5));
    q.addBindValue(review.tableId);
    q.addBindValue(review.dishId);
    q.addBindValue(review.comment);
    q.addBindValue(review.managerReply);
    return q.exec();
}

bool DatabaseManager::addManagerReply(int reviewId, const QString& reply) {
    QSqlQuery q(db_);
    q.prepare("UPDATE reviews SET manager_reply = ? WHERE id = ?");
    q.addBindValue(reply);
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

std::vector<TableRecommendation> DatabaseManager::recommendTables(const TablePreference& pref) const {
    std::vector<TableRecommendation> recs;
    for (const auto& t : getAllTables()) {
        if (t.capacity < pref.partySize) continue;
        if (!isTableAvailable(t.id, pref.visitDate, pref.startHour, pref.durationHours)) continue;

        TableRecommendation rec;
        rec.table = t;
        rec.avgRating = getAverageTableRating(t.id);
        int score = 50;

        int extra = t.capacity - pref.partySize;
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

        if (rec.avgRating >= 4.5) {
            score += 8;
            rec.reasons << QString("Высокий рейтинг гостей (★ %1/5.0)").arg(rec.avgRating, 0, 'f', 1);
        }

        rec.matchScore = std::clamp(score, 10, 99);
        if (rec.reasons.isEmpty()) rec.reasons << "Свободный подходящий столик";
        recs.push_back(rec);
    }

    std::sort(recs.begin(), recs.end(), [](const TableRecommendation& a, const TableRecommendation& b) {
        if (a.matchScore != b.matchScore) return a.matchScore > b.matchScore;
        return a.avgRating > b.avgRating;
    });
    return recs;
}

std::vector<MenuSetRecommendation> DatabaseManager::recommendMenuSets(const MenuPreference& pref) const {
    std::vector<MenuSetRecommendation> sets;
    auto allDishes = getAllDishes();

    auto matchesPref = [&pref](const DishInfo& d) {
        if (pref.avoidSpicy && d.tags.contains("острое")) return false;
        if (!pref.dietaryTag.isEmpty() && pref.dietaryTag != "Все" && !d.tags.contains(pref.dietaryTag)) return false;
        return true;
    };

    std::vector<DishInfo> starters, mains, desserts, drinks;
    for (const auto& d : allDishes) {
        if (!matchesPref(d)) continue;
        if (d.category == "Закуски" || d.category == "Супы") starters.push_back(d);
        else if (d.category == "Горячее") mains.push_back(d);
        else if (d.category == "Десерты") desserts.push_back(d);
        else if (d.category == "Напитки") drinks.push_back(d);
    }

    if (drinks.empty()) {
        for (const auto& d : allDishes) if (d.category == "Напитки") drinks.push_back(d);
    }
    if (desserts.empty()) {
        for (const auto& d : allDishes) if (d.category == "Десерты") desserts.push_back(d);
    }

    for (size_t i = 0; i < mains.size() && sets.size() < 3; ++i) {
        MenuSetRecommendation s;
        s.dishes.push_back(mains[i]);
        s.totalPrice += mains[i].price;
        s.totalCalories += mains[i].calories;

        for (const auto& st : starters) {
            if (s.totalPrice + st.price <= pref.maxBudget * 0.85 &&
                (pref.maxCalories <= 0 || s.totalCalories + st.calories <= pref.maxCalories)) {
                s.dishes.insert(s.dishes.begin(), st);
                s.totalPrice += st.price;
                s.totalCalories += st.calories;
                break;
            }
        }
        for (const auto& dr : drinks) {
            if (s.totalPrice + dr.price <= pref.maxBudget &&
                (pref.maxCalories <= 0 || s.totalCalories + dr.calories <= pref.maxCalories)) {
                s.dishes.push_back(dr);
                s.totalPrice += dr.price;
                s.totalCalories += dr.calories;
                break;
            }
        }
        for (const auto& ds : desserts) {
            if (s.totalPrice + ds.price <= pref.maxBudget &&
                (pref.maxCalories <= 0 || s.totalCalories + ds.calories <= pref.maxCalories)) {
                s.dishes.push_back(ds);
                s.totalPrice += ds.price;
                s.totalCalories += ds.calories;
                break;
            }
        }

        if (s.totalPrice <= pref.maxBudget) {
            if (sets.empty()) {
                s.title = "Сет №1 «Выбор Шефа и Гостей»";
                s.explanation = "Сбалансированный курс из блюд с максимальным рейтингом";
            } else if (sets.size() == 1) {
                s.title = "Сет №2 «Гастрономическая гармония»";
                s.explanation = "Авторское сочетание вкусов в рамках вашего бюджета";
            } else {
                s.title = "Сет №3 «Легкий вечер»";
                s.explanation = "Оптимальный баланс калорийности и скорости подачи";
            }
            sets.push_back(s);
        }
    }
    return sets;
}

} // namespace Restaurant
