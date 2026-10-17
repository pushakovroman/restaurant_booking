#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "DatabaseManager.hpp"
#include "FloorMapWidget.hpp"
#include <QMainWindow>
#include <QTabWidget>
#include <QDateEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <vector>

namespace Restaurant {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(DatabaseManager& db, QWidget* parent = nullptr);

private slots:
    void onDateTimeFilterChanged();
    void onTableSelectedOnMap(int tableId);
    void onQuickBookFromMap();
    void onQuickWalkInFromMap();

    // Вкладка Бронирования и Хостес
    void onAddDishToPreorder();
    void onRemoveSelectedPreorderItem();
    void onClearPreorder();
    void onCreateReservationClicked();
    void onCheckInClicked();
    void onCheckOutClicked();
    void onCancelReservationClicked();
    void onRescheduleReservationClicked();
    void onEditReservationCommentClicked();
    void onShowReceiptClicked();
    void onExportReceiptClicked();
    void onAddWaitlistClicked();
    void onSeatFromWaitlistClicked();
    void onRemoveFromWaitlistClicked();

    // Вкладка Умных рекомендаций
    void onRunTableRecommender();
    void onSelectRecommendedTable(int row, int col);
    void onRunMenuRecommender();
    void onApplyRecommendedMenuSet();

    // Вкладка Меню
    void onFilterMenuChanged();
    void onAddNewDishClicked();
    void onEditDishClicked();
    void onDeleteDishClicked();

    // Вкладка Отзывов и Аналитики
    void onAddReviewClicked();
    void onReplyToReviewClicked();
    void onDeleteReviewClicked();
    void onExportAnalyticsReportClicked();

private:
    DatabaseManager& db_;
    int currentTableId_ = 1;
    std::vector<PreorderItem> currentPreorder_;
    std::vector<TableRecommendation> lastTableRecs_;
    std::vector<MenuSetRecommendation> lastMenuSets_;

    // Верхняя панель управления датой и временем
    QDateEdit* dateEdit_ = nullptr;
    QSpinBox* hourSpin_ = nullptr;
    QSpinBox* durationSpin_ = nullptr;
    QLabel* summaryBannerLabel_ = nullptr;

    QTabWidget* mainTabs_ = nullptr;

    // 1. Вкладка 2D-карты зала
    FloorMapWidget* floorMapWidget_ = nullptr;
    QLabel* cardTitleLabel_ = nullptr;
    QLabel* cardZoneLabel_ = nullptr;
    QLabel* cardFeaturesLabel_ = nullptr;
    QLabel* cardDepositLabel_ = nullptr;
    QLabel* cardRatingLabel_ = nullptr;
    QLabel* cardScheduleLabel_ = nullptr;
    QListWidget* cardReviewsList_ = nullptr;

    // 2. Вкладка Онлайн-записи и Хостес
    QComboBox* bookingTypeCombo_ = nullptr;
    QLineEdit* guestNameEdit_ = nullptr;
    QLineEdit* guestPhoneEdit_ = nullptr;
    QSpinBox* partySizeSpin_ = nullptr;
    QComboBox* tableSelectCombo_ = nullptr;
    QComboBox* occasionCombo_ = nullptr;
    QPlainTextEdit* bookingCommentEdit_ = nullptr;
    QComboBox* preorderDishCombo_ = nullptr;
    QSpinBox* preorderQtySpin_ = nullptr;
    QListWidget* preorderListWidget_ = nullptr;
    QLabel* preorderTotalLabel_ = nullptr;
    QLineEdit* reservationSearchEdit_ = nullptr;
    QTableWidget* reservationsTable_ = nullptr;
    QTableWidget* waitlistTable_ = nullptr;

    // 3. Вкладка Умных рекомендаций
    QSpinBox* recPartySpin_ = nullptr;
    QComboBox* recOccasionCombo_ = nullptr;
    QComboBox* recZoneCombo_ = nullptr;
    QCheckBox* recWindowCheck_ = nullptr;
    QCheckBox* recQuietCheck_ = nullptr;
    QCheckBox* recOutletCheck_ = nullptr;
    QCheckBox* recSofaCheck_ = nullptr;
    QTableWidget* recTablesWidget_ = nullptr;

    QDoubleSpinBox* recBudgetSpin_ = nullptr;
    QComboBox* recDietCombo_ = nullptr;
    QCheckBox* recNoSpicyCheck_ = nullptr;
    QSpinBox* recMaxCalSpin_ = nullptr;
    QListWidget* recMenuSetsList_ = nullptr;

    // 4. Вкладка Меню
    QComboBox* menuCategoryFilter_ = nullptr;
    QComboBox* menuTagFilter_ = nullptr;
    QLineEdit* menuSearchEdit_ = nullptr;
    QDoubleSpinBox* menuMaxPriceSpin_ = nullptr;
    QTableWidget* menuTableWidget_ = nullptr;

    // 5. Вкладка Отзывов и Аналитики
    QComboBox* revFilterTableCombo_ = nullptr;
    QComboBox* revFilterDishCombo_ = nullptr;
    QTableWidget* reviewsTable_ = nullptr;
    QLineEdit* revAuthorEdit_ = nullptr;
    QSpinBox* revRatingSpin_ = nullptr;
    QComboBox* revTableCombo_ = nullptr;
    QComboBox* revDishCombo_ = nullptr;
    QLineEdit* revCommentEdit_ = nullptr;
    QLabel* analyticsSummaryLabel_ = nullptr;
    std::vector<QProgressBar*> hourlyBars_;

    void setupUi();
    QWidget* createFloorMapTab();
    QWidget* createBookingTab();
    QWidget* createRecommenderTab();
    QWidget* createMenuTab();
    QWidget* createReviewsAndAnalyticsTab();

    void reloadDishCombos();
    void refreshAllViews();
    void refreshFloorMapAndCard();
    void refreshReservationsAndWaitlist();
    void refreshPreorderWidget();
    void refreshMenuTable();
    void refreshReviewsAndAnalytics();

    QString activeDateString() const;
    int selectedReservationIdFromTable() const;
    int selectedWaitlistIdFromTable() const;
    int selectedDishIdFromTable() const;
};

} // namespace Restaurant

#endif // MAIN_WINDOW_HPP
