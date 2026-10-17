#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "DatabaseManager.hpp"
#include "FloorMapWidget.hpp"
#include "AnalyticsChartWidget.hpp"
#include <QMainWindow>
#include <QStackedWidget>
#include <QDateEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <vector>

namespace Restaurant {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(DatabaseManager& db, QWidget* parent = nullptr);

    static QString darkLuxuryStyleSheet();
    static QString lightCafeStyleSheet();

private slots:
    void onDateTimeFilterChanged();
    void onTimeSliderChanged(int value);
    void onToggleThemeClicked();
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
    bool isDarkTheme_ = true;
    int currentTableId_ = 1;
    std::vector<PreorderItem> currentPreorder_;
    std::vector<TableRecommendation> lastTableRecs_;
    std::vector<MenuSetRecommendation> lastMenuSets_;

    // Левая боковая панель навигации (Sidebar) и стек экранов
    QListWidget* sidebarNav_ = nullptr;
    QStackedWidget* pagesStack_ = nullptr;
    QPushButton* themeToggleBtn_ = nullptr;

    // Верхние KPI-карточки и панель «Машины времени»
    QLabel* kpiFreeTablesLabel_ = nullptr;
    QLabel* kpiSeatedGuestsLabel_ = nullptr;
    QLabel* kpiWaitlistLabel_ = nullptr;
    QLabel* kpiRevenueRatingLabel_ = nullptr;

    QDateEdit* dateEdit_ = nullptr;
    QSpinBox* hourSpin_ = nullptr;
    QSlider* timeSlider_ = nullptr;
    QSpinBox* durationSpin_ = nullptr;

    // 1. Экран 2D-карты зала
    FloorMapWidget* floorMapWidget_ = nullptr;
    QLabel* cardTitleLabel_ = nullptr;
    QLabel* cardZoneLabel_ = nullptr;
    QLabel* cardFeaturesLabel_ = nullptr;
    QLabel* cardDepositLabel_ = nullptr;
    QLabel* cardRatingLabel_ = nullptr;
    QLabel* cardScheduleLabel_ = nullptr;
    QListWidget* cardReviewsList_ = nullptr;

    // 2. Экран Онлайн-записи и Хостес
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

    // 3. Экран Умных рекомендаций
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

    // 4. Экран Меню (BYN)
    QComboBox* menuCategoryFilter_ = nullptr;
    QComboBox* menuTagFilter_ = nullptr;
    QLineEdit* menuSearchEdit_ = nullptr;
    QDoubleSpinBox* menuMaxPriceSpin_ = nullptr;
    QTableWidget* menuTableWidget_ = nullptr;

    // 5. Экран Отзывов и Аналитики
    QComboBox* revFilterTableCombo_ = nullptr;
    QComboBox* revFilterDishCombo_ = nullptr;
    QTableWidget* reviewsTable_ = nullptr;
    QLineEdit* revAuthorEdit_ = nullptr;
    QSpinBox* revRatingSpin_ = nullptr;
    QComboBox* revTableCombo_ = nullptr;
    QComboBox* revDishCombo_ = nullptr;
    QLineEdit* revCommentEdit_ = nullptr;
    QLabel* analyticsSummaryLabel_ = nullptr;
    AnalyticsChartWidget* analyticsChartWidget_ = nullptr;

    void setupUi();
    QFrame* createKpiCard(const QString& title, QLabel*& outValueLabel, const QString& accentColor);
    QWidget* createFloorMapPage();
    QWidget* createBookingPage();
    QWidget* createRecommenderPage();
    QWidget* createMenuPage();
    QWidget* createReviewsAndAnalyticsPage();

    void switchToPage(int index);
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
