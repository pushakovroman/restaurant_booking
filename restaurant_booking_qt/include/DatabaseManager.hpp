#ifndef DATABASE_MANAGER_HPP
#define DATABASE_MANAGER_HPP

#include "Models.hpp"
#include "SchemaSql.hpp"
#include <QMap>
#include <QSqlDatabase>
#include <QString>
#include <vector>

class QSqlQuery;

namespace Restaurant {

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool openDatabase(const QString& dbFilePath);
    void closeDatabase();
    bool isOpen() const { return db_.isOpen(); }
    QString lastError() const { return lastError_; }

    // Столики и 2D-карта зала
    std::vector<TableInfo> getAllTables() const;
    TableInfo getTableById(int id) const;
    // 0 = Свободен, 1 = Забронирован (Confirmed), 2 = Занят в ресторане (Seated)
    int getTableOccupancyStatus(int tableId, const QString& date, int startHour, int durationHours) const;
    // Те же статусы для всех столов одним запросом (для отрисовки карты зала). Отсутствие ключа = свободен.
    QMap<int, int> getOccupancyMap(const QString& date, int startHour, int durationHours) const;
    // Статус стола по каждому часу рабочего дня (индекс 0 = OPEN_HOUR)
    std::vector<int> getTableDayStatuses(int tableId, const QString& date) const;
    // Сколько столов занято в каждый час рабочего дня (индекс 0 = OPEN_HOUR)
    std::vector<int> getBusyTableCountsByHour(const QString& date) const;
    bool isTableAvailable(int tableId, const QString& date, int startHour, int durationHours, int ignoreReservationId = 0) const;
    int getFreeHoursFrom(int tableId, const QString& date, int startHour) const;

    // Меню и блюда
    std::vector<DishInfo> getAllDishes() const;
    DishInfo getDishById(int id) const;
    std::vector<DishInfo> searchDishes(const QString& category,
                                       const QString& tag,
                                       const QString& keyword,
                                       double maxPrice) const;
    bool addDish(const DishInfo& dish, QString* errorOut = nullptr);
    double calculatePreorderTotal(const std::vector<PreorderItem>& items, bool isOnline) const;

    // Бронирования (Онлайн + В ресторане Walk-in)
    std::vector<ReservationInfo> getReservationsByDate(const QString& date) const;
    std::vector<ReservationInfo> getAllReservations() const;
    ReservationInfo getReservationById(int id) const;
    // Все проверки и вставка выполняются в одной транзакции (BEGIN IMMEDIATE)
    ReservationResult createReservation(const ReservationInfo& res);
    // Допустимые переходы: Confirmed -> Seated | Cancelled, Seated -> Completed
    bool updateReservationStatus(int reservationId, const QString& newStatus, QString* errorOut = nullptr);
    bool updateReservationComment(int reservationId, const QString& newComment);
    // Строки-предупреждения, если в комментарии указана аллергия, а в предзаказе есть блюдо с таким аллергеном
    std::vector<QString> findAllergenConflicts(const QString& comment, const std::vector<PreorderItem>& preorder) const;

    // Лист ожидания (Waitlist), порядок — по очереди поступления
    std::vector<WaitlistInfo> getWaitlist() const;
    bool addToWaitlist(const WaitlistInfo& entry, QString* errorOut = nullptr);
    bool removeFromWaitlist(int waitlistId);

    // Отзывы, комментарии и рейтинги
    std::vector<ReviewInfo> getAllReviews() const;
    bool addReview(const ReviewInfo& review, QString* errorOut = nullptr);
    bool addManagerReply(int reviewId, const QString& reply);
    double getAverageTableRating(int tableId) const;
    QMap<int, double> getAverageTableRatings() const;
    double getAverageDishRating(int dishId) const;
    double getOverallRating() const;

    // Умные рекомендации под запрос
    std::vector<TableRecommendation> recommendTables(const TablePreference& pref) const;
    std::vector<MenuSetRecommendation> recommendMenuSets(const MenuPreference& pref) const;

    // Проверка введённых данных (используются и интерфейсом)
    static bool isValidGuestName(const QString& name);
    static bool isValidPhone(const QString& phone);

    static constexpr int OPEN_HOUR = Schema::kOpenHour;
    static constexpr int CLOSE_HOUR = Schema::kCloseHour;
    static constexpr int MAX_DURATION_HOURS = 4;
    static constexpr int MAX_COMMENT_LENGTH = 500;

private:
    struct ActiveSlot {
        int tableId;
        int startHour;
        int durationHours;
        int status;     // 1 = Confirmed, 2 = Seated
    };

    QSqlDatabase db_;
    QString lastError_;

    bool initializeSchemaAndSeedData();
    bool columnExists(const QString& table, const QString& column) const;
    bool seedDemoData();
    std::vector<ActiveSlot> loadActiveSlots(const QString& date) const;
    std::vector<PreorderItem> loadPreorderForReservation(int reservationId) const;
    ReservationInfo readReservationRow(const QSqlQuery& q) const;
    static bool isValidDateString(const QString& date);
};

} // namespace Restaurant

#endif // DATABASE_MANAGER_HPP
