#ifndef MODELS_HPP
#define MODELS_HPP

#include <QString>
#include <QStringList>
#include <vector>

namespace Restaurant {

struct TableInfo {
    int id = 0;
    QString code;           // "W1", "T1", "M1", "V1", "B1"
    QString zone;           // "Панорамные окна", "Летняя терраса", "Основной зал", "VIP-каминная", "Барная зона"
    int capacity = 2;
    int gridX = 0;          // 0..3
    int gridY = 0;          // 0..2
    bool hasOutlet = false;
    bool isQuiet = false;
    bool hasSofa = false;
    bool nearWindow = false;
    double minDeposit = 0.0;
    QString description;
};

struct DishInfo {
    int id = 0;
    QString name;
    QString category;       // "Закуски", "Супы", "Горячее", "Десерты", "Напитки"
    double price = 0.0;
    int weightGrams = 0;
    int calories = 0;
    int prepTimeMin = 15;
    QStringList tags;       // "вегетарианское", "без глютена", "морепродукты", "хит шефа", "легкое", "острое"
    QString description;
    QStringList allergens;  // см. allergenVocabulary(): "глютен", "молоко", "орехи", ...
};

struct PreorderItem {
    int dishId = 0;
    QString dishName;
    double unitPrice = 0.0;
    int quantity = 1;
};

struct ReservationInfo {
    int id = 0;
    QString bookingCode;    // "BK-1001"
    QString guestName;
    QString guestPhone;
    QString visitDate;      // "YYYY-MM-DD"
    int startHour = 19;     // 10..22
    int durationHours = 2;  // 1..4
    int partySize = 2;
    int tableId = 1;
    QString bookingType;    // "Online" или "WalkIn"
    QString status;         // "Confirmed", "Seated", "Completed", "Cancelled"
    QString occasion;       // "Свидание", "Деловая встреча", "День рождения", "Семейный ужин", "Обычный визит"
    QString comment;
    double depositPaid = 0.0;   // внесённый депозит (для столов с min_deposit > 0)
    std::vector<PreorderItem> preorder;
};

struct ReviewInfo {
    int id = 0;
    QString authorName;
    QString reviewDate;
    int rating = 5;         // 1..5
    int tableId = 0;
    int dishId = 0;
    QString comment;
    QString managerReply;
};

struct WaitlistInfo {
    int id = 0;
    QString guestName;
    QString guestPhone;
    QString visitDate;
    int preferredHour = 19;
    int partySize = 2;
    QString comment;
};

struct TablePreference {
    int partySize = 2;
    QString visitDate;      // "YYYY-MM-DD", заполняется вызывающим кодом
    int startHour = 19;
    int durationHours = 2;
    QString occasion = "Свидание";
    bool preferWindow = false;
    bool preferQuiet = false;
    bool needOutlet = false;
    bool preferSofa = false;
};

struct TableRecommendation {
    TableInfo table;
    int matchScore = 0;     // 0..100
    double avgRating = 0.0;
    QStringList reasons;
};

struct MenuPreference {
    double maxBudget = 2500.0;
    QString dietaryTag;     // "", "вегетарианское", "без глютена", "морепродукты", "легкое"
    bool avoidSpicy = false;
    int maxCalories = 0;
};

struct MenuSetRecommendation {
    QString title;
    std::vector<DishInfo> dishes;
    double totalPrice = 0.0;
    int totalCalories = 0;
    QString explanation;
};

// Результат попытки создать бронь: вместо безликого «0» вызывающий код получает причину отказа
enum class ReservationError {
    None,
    InvalidGuestName,
    InvalidPhone,
    InvalidPartySize,
    InvalidDate,
    InvalidTime,
    PastDateTime,
    WalkInNotToday,
    InvalidField,
    TableNotFound,
    CapacityExceeded,
    SlotTaken,
    DepositRequired,
    InvalidPreorder,
    DatabaseError
};

struct ReservationResult {
    bool ok = false;
    int id = 0;
    QString bookingCode;
    ReservationError error = ReservationError::None;
    QString message;        // готовое сообщение для пользователя
};

// Человекочитаемое название статуса брони (в БД хранятся английские коды)
inline QString reservationStatusLabel(const QString& status) {
    if (status == QLatin1String("Confirmed")) return QStringLiteral("Подтверждена");
    if (status == QLatin1String("Seated"))    return QStringLiteral("Гости за столом");
    if (status == QLatin1String("Completed")) return QStringLiteral("Завершена");
    if (status == QLatin1String("Cancelled")) return QStringLiteral("Отменена");
    return status;
}

// Допустимые значения аллергенов у блюд (хранятся в dishes.allergens через запятую)
inline QStringList allergenVocabulary() {
    return {QStringLiteral("глютен"), QStringLiteral("молоко"), QStringLiteral("яйца"),
            QStringLiteral("орехи"), QStringLiteral("рыба"), QStringLiteral("ракообразные"),
            QStringLiteral("моллюски"), QStringLiteral("соя"), QStringLiteral("кунжут")};
}

} // namespace Restaurant

#endif // MODELS_HPP
