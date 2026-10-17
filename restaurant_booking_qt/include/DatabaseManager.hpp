#ifndef DATABASE_MANAGER_HPP
#define DATABASE_MANAGER_HPP

#include "Models.hpp"
#include <QSqlDatabase>
#include <QString>
#include <QMap>
#include <vector>

namespace Restaurant {

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool openDatabase(const QString& dbFilePath);
    void closeDatabase();

    // Столики и 2D-карта зала
    std::vector<TableInfo> getAllTables() const;
    TableInfo getTableById(int id) const;
    // 0 = Свободен, 1 = Забронирован (Confirmed), 2 = Занят в ресторане (Seated)
    int getTableOccupancyStatus(int tableId, const QString& date, int startHour, int durationHours) const;
    QMap<int, QString> getActiveGuestBadgeByTable(const QString& date, int startHour, int durationHours) const;
    bool isTableAvailable(int tableId, const QString& date, int startHour, int durationHours, int ignoreReservationId = 0) const;
    int getFreeHoursFrom(int tableId, const QString& date, int startHour) const;

    // Меню и блюда
    std::vector<DishInfo> getAllDishes() const;
    DishInfo getDishById(int id) const;
    std::vector<DishInfo> searchDishes(const QString& category,
                                       const QString& tag,
                                       const QString& keyword,
                                       double maxPrice) const;
    bool addDish(DishInfo& dish);
    bool updateDish(const DishInfo& dish);
    bool deleteDish(int dishId);
    double calculatePreorderTotal(const std::vector<PreorderItem>& items, bool isOnline) const;

    // Бронирования (Онлайн + В ресторане Walk-in)
    std::vector<ReservationInfo> getReservationsByDate(const QString& date, const QString& searchFilter = "") const;
    std::vector<ReservationInfo> getAllReservations() const;
    ReservationInfo getReservationById(int id) const;
    int createReservation(const ReservationInfo& res, QString& outBookingCode, QString* outError = nullptr);
    bool updateReservationStatus(int reservationId, const QString& newStatus, QString* outError = nullptr);
    bool updateReservationComment(int reservationId, const QString& newComment);
    bool rescheduleReservation(int reservationId,
                               int newTableId,
                               const QString& newDate,
                               int newStartHour,
                               int newDurationHours,
                               QString* outError = nullptr);

    // Лист ожидания (Waitlist)
    std::vector<WaitlistInfo> getWaitlist() const;
    bool addToWaitlist(const WaitlistInfo& entry);
    bool removeFromWaitlist(int waitlistId);
    int seatFromWaitlist(int waitlistId, int preferredTableId, QString& outBookingCode, QString* outError = nullptr);

    // Отзывы, комментарии и рейтинги
    std::vector<ReviewInfo> getAllReviews(int filterTableId = 0, int filterDishId = 0) const;
    bool addReview(const ReviewInfo& review);
    bool addManagerReply(int reviewId, const QString& reply);
    bool deleteReview(int reviewId);
    double getAverageTableRating(int tableId) const;
    double getAverageDishRating(int dishId) const;
    double getOverallRating() const;

    // Умные рекомендации под запрос
    std::vector<TableRecommendation> recommendTables(const TablePreference& pref) const;
    std::vector<MenuSetRecommendation> recommendMenuSets(const MenuPreference& pref) const;

    // Генерация текстовых отчетов и квитанций для экспорта
    QString formatReservationReceipt(int reservationId) const;
    QString formatDailyAnalyticsReport(const QString& date) const;

    static constexpr int OPEN_HOUR = 10;
    static constexpr int CLOSE_HOUR = 23;

private:
    QSqlDatabase db_;
    QString connectionName_ = "restaurant_sql_conn";
    void initializeSchemaAndSeedData();
    std::vector<PreorderItem> loadPreorderForReservation(int reservationId) const;
};

} // namespace Restaurant

#endif // DATABASE_MANAGER_HPP
