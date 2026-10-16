#include "MainWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QDate>
#include <QSplitter>

namespace Restaurant {

MainWindow::MainWindow(DatabaseManager& db, QWidget* parent)
    : QMainWindow(parent), db_(db) {
    setWindowTitle("Система бронирования в ресторане «Aura Gastronomy» — Qt GUI + SQLite");
    resize(1340, 840);
    setupUi();
    refreshAllViews();
    onRunTableRecommender();
    onRunMenuRecommender();
}

QString MainWindow::activeDateString() const {
    return dateEdit_->date().toString("yyyy-MM-dd");
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    QVBoxLayout* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(12, 10, 12, 10);
    rootLayout->setSpacing(10);

    // Верхняя панель фильтрации даты/времени и оперативной сводки
    QGroupBox* topBar = new QGroupBox("Параметры смены и просматриваемого интервала времени");
    QHBoxLayout* topLayout = new QHBoxLayout(topBar);

    topLayout->addWidget(new QLabel("Дата визита:"));
    dateEdit_ = new QDateEdit(QDate(2026, 10, 16));
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    topLayout->addWidget(dateEdit_);

    topLayout->addSpacing(12);
    topLayout->addWidget(new QLabel("Час начала (10..22):"));
    hourSpin_ = new QSpinBox();
    hourSpin_->setRange(DatabaseManager::OPEN_HOUR, DatabaseManager::CLOSE_HOUR - 1);
    hourSpin_->setValue(19);
    hourSpin_->setSuffix(":00");
    topLayout->addWidget(hourSpin_);

    topLayout->addSpacing(12);
    topLayout->addWidget(new QLabel("Длительность:"));
    durationSpin_ = new QSpinBox();
    durationSpin_->setRange(1, 4);
    durationSpin_->setValue(2);
    durationSpin_->setSuffix(" ч.");
    topLayout->addWidget(durationSpin_);

    topLayout->addSpacing(24);
    summaryBannerLabel_ = new QLabel();
    summaryBannerLabel_->setStyleSheet("font-weight: bold; color: #F5D76E; font-size: 13px;");
    topLayout->addWidget(summaryBannerLabel_, 1);

    QPushButton* refreshBtn = new QPushButton("Обновить данные");
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshAllViews);
    topLayout->addWidget(refreshBtn);

    connect(dateEdit_, &QDateEdit::dateChanged, this, &MainWindow::onDateTimeFilterChanged);
    connect(hourSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onDateTimeFilterChanged);
    connect(durationSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onDateTimeFilterChanged);

    rootLayout->addWidget(topBar);

    // Главные вкладки приложения
    mainTabs_ = new QTabWidget();
    mainTabs_->addTab(createFloorMapTab(), "🗺 Интерактивная 2D-карта зала");
    mainTabs_->addTab(createBookingTab(), "📅 Онлайн-запись + В ресторане (Хостес)");
    mainTabs_->addTab(createRecommenderTab(), "✨ Рекомендации под твой запрос");
    mainTabs_->addTab(createMenuTab(), "🍽 Меню и предзаказ");
    mainTabs_->addTab(createReviewsAndAnalyticsTab(), "💬 Комментарии, отзывы и Аналитика");

    rootLayout->addWidget(mainTabs_, 1);
    setCentralWidget(central);
}

QWidget* MainWindow::createFloorMapTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    floorMapWidget_ = new FloorMapWidget();
    connect(floorMapWidget_, &FloorMapWidget::tableSelected, this, &MainWindow::onTableSelectedOnMap);
    layout->addWidget(floorMapWidget_, 3);

    // Правая панель: Детальная карточка выбранного стола
    QGroupBox* sideBox = new QGroupBox("Карточка выбранного столика на 2D-карте");
    QVBoxLayout* sideLayout = new QVBoxLayout(sideBox);

    cardTitleLabel_ = new QLabel();
    cardTitleLabel_->setStyleSheet("font-size: 16px; font-weight: bold; color: #F5D76E;");
    cardTitleLabel_->setWordWrap(true);
    sideLayout->addWidget(cardTitleLabel_);

    cardZoneLabel_ = new QLabel();
    cardZoneLabel_->setWordWrap(true);
    sideLayout->addWidget(cardZoneLabel_);

    cardFeaturesLabel_ = new QLabel();
    cardFeaturesLabel_->setWordWrap(true);
    sideLayout->addWidget(cardFeaturesLabel_);

    cardDepositLabel_ = new QLabel();
    sideLayout->addWidget(cardDepositLabel_);

    cardRatingLabel_ = new QLabel();
    cardRatingLabel_->setStyleSheet("color: #2ECC71; font-weight: bold;");
    sideLayout->addWidget(cardRatingLabel_);

    cardScheduleLabel_ = new QLabel();
    cardScheduleLabel_->setWordWrap(true);
    cardScheduleLabel_->setStyleSheet("background-color: #181A24; padding: 8px; border-radius: 6px; font-family: monospace;");
    sideLayout->addWidget(cardScheduleLabel_);

    sideLayout->addWidget(new QLabel("Отзывы гостей об этом столике:"));
    cardReviewsList_ = new QListWidget();
    sideLayout->addWidget(cardReviewsList_, 1);

    QPushButton* btnBookOnline = new QPushButton("Оформить Онлайн-бронь этого столика");
    btnBookOnline->setStyleSheet("background-color: #2980B9; color: white; font-weight: bold; padding: 8px;");
    connect(btnBookOnline, &QPushButton::clicked, this, &MainWindow::onQuickBookFromMap);
    sideLayout->addWidget(btnBookOnline);

    QPushButton* btnSeatWalkIn = new QPushButton("Посадить гостей сейчас (В ресторане / Walk-in)");
    btnSeatWalkIn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold; padding: 8px;");
    connect(btnSeatWalkIn, &QPushButton::clicked, this, &MainWindow::onQuickWalkInFromMap);
    sideLayout->addWidget(btnSeatWalkIn);

    layout->addWidget(sideBox, 2);
    return tab;
}

QWidget* MainWindow::createBookingTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    // Левая колонка: форма создания брони или посадки Walk-in
    QGroupBox* formBox = new QGroupBox("Оформление записи (Онлайн или В ресторане)");
    QVBoxLayout* formLayout = new QVBoxLayout(formBox);

    QFormLayout* fields = new QFormLayout();
    bookingTypeCombo_ = new QComboBox();
    bookingTypeCombo_->addItem("Онлайн-запись заранее (скидка 5% на предзаказ)", "Online");
    bookingTypeCombo_->addItem("В ресторане / Хостес (мгновенная посадка Walk-in)", "WalkIn");
    connect(bookingTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::refreshPreorderWidget);
    fields->addRow("Режим записи:", bookingTypeCombo_);

    guestNameEdit_ = new QLineEdit("Анна Морозова");
    fields->addRow("Имя гостя:", guestNameEdit_);

    guestPhoneEdit_ = new QLineEdit("+7 (915) 222-33-44");
    fields->addRow("Телефон:", guestPhoneEdit_);

    partySizeSpin_ = new QSpinBox();
    partySizeSpin_->setRange(1, 12);
    partySizeSpin_->setValue(2);
    fields->addRow("Количество гостей:", partySizeSpin_);

    tableSelectCombo_ = new QComboBox();
    for (const auto& t : db_.getAllTables()) {
        tableSelectCombo_->addItem(
            QString("Стол #%1 [%2] — %3 (до %4 чел.)").arg(t.id).arg(t.code).arg(t.zone).arg(t.capacity),
            t.id);
    }
    fields->addRow("Выбор столика:", tableSelectCombo_);

    occasionCombo_ = new QComboBox();
    occasionCombo_->addItems({"Свидание", "Деловая встреча", "День рождения", "Семейный ужин", "Обычный визит"});
    fields->addRow("Повод визита:", occasionCombo_);

    formLayout->addLayout(fields);

    formLayout->addWidget(new QLabel("Комментарий к брони (пожелания, аллергии, подача блюд):"));
    bookingCommentEdit_ = new QPlainTextEdit();
    bookingCommentEdit_->setPlaceholderText("Например: столик у окна, свечи, аллергия на орехи...");
    bookingCommentEdit_->setMaximumHeight(65);
    formLayout->addWidget(bookingCommentEdit_);

    // Корзина предзаказа по меню
    QGroupBox* preBox = new QGroupBox("Предзаказ блюд к приходу гостей (при онлайн-записи скидка 5%)");
    QVBoxLayout* preLayout = new QVBoxLayout(preBox);

    QHBoxLayout* addDishRow = new QHBoxLayout();
    preorderDishCombo_ = new QComboBox();
    for (const auto& d : db_.getAllDishes()) {
        preorderDishCombo_->addItem(QString("%1 (%2 руб.)").arg(d.name).arg(d.price, 0, 'f', 0), d.id);
    }
    addDishRow->addWidget(preorderDishCombo_, 1);

    preorderQtySpin_ = new QSpinBox();
    preorderQtySpin_->setRange(1, 20);
    preorderQtySpin_->setValue(1);
    addDishRow->addWidget(preorderQtySpin_);

    QPushButton* addDishBtn = new QPushButton("+ Добавить");
    connect(addDishBtn, &QPushButton::clicked, this, &MainWindow::onAddDishToPreorder);
    addDishRow->addWidget(addDishBtn);

    QPushButton* clearPreBtn = new QPushButton("Очистить");
    connect(clearPreBtn, &QPushButton::clicked, this, &MainWindow::onClearPreorder);
    addDishRow->addWidget(clearPreBtn);

    preLayout->addLayout(addDishRow);

    preorderListWidget_ = new QListWidget();
    preLayout->addWidget(preorderListWidget_);

    preorderTotalLabel_ = new QLabel("Итого по предзаказу: 0 руб.");
    preorderTotalLabel_->setStyleSheet("font-weight: bold; color: #F5D76E;");
    preLayout->addWidget(preorderTotalLabel_);

    formLayout->addWidget(preBox);

    QPushButton* submitBtn = new QPushButton("Подтвердить запись / Посадить гостей");
    submitBtn->setStyleSheet("background-color: #D4AF37; color: #181A24; font-weight: bold; font-size: 14px; padding: 10px;");
    connect(submitBtn, &QPushButton::clicked, this, &MainWindow::onCreateReservationClicked);
    formLayout->addWidget(submitBtn);

    QPushButton* waitlistBtn = new QPushButton("Записать гостя в Лист ожидания (Waitlist)");
    connect(waitlistBtn, &QPushButton::clicked, this, &MainWindow::onAddWaitlistClicked);
    formLayout->addWidget(waitlistBtn);

    layout->addWidget(formBox, 2);

    // Правая колонка: таблица броней и управление статусами Хостес
    QGroupBox* listBox = new QGroupBox("Журнал бронирований и посадки на выбранную дату");
    QVBoxLayout* listLayout = new QVBoxLayout(listBox);

    reservationsTable_ = new QTableWidget(0, 8);
    reservationsTable_->setHorizontalHeaderLabels(
        {"ID", "Код", "Время", "Стол", "Гость", "Персон", "Тип", "Статус"});
    reservationsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    reservationsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    reservationsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listLayout->addWidget(reservationsTable_, 3);

    QHBoxLayout* actionRow = new QHBoxLayout();
    QPushButton* btnCheckIn = new QPushButton("Посадить за стол (Check-in)");
    btnCheckIn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold;");
    connect(btnCheckIn, &QPushButton::clicked, this, &MainWindow::onCheckInClicked);
    actionRow->addWidget(btnCheckIn);

    QPushButton* btnCheckOut = new QPushButton("Закрыть счет (Check-out)");
    btnCheckOut->setStyleSheet("background-color: #2980B9; color: white; font-weight: bold;");
    connect(btnCheckOut, &QPushButton::clicked, this, &MainWindow::onCheckOutClicked);
    actionRow->addWidget(btnCheckOut);

    QPushButton* btnCancel = new QPushButton("Отменить бронь");
    btnCancel->setStyleSheet("background-color: #C0392B; color: white; font-weight: bold;");
    connect(btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelReservationClicked);
    actionRow->addWidget(btnCancel);

    QPushButton* btnComment = new QPushButton("Изменить комментарий");
    connect(btnComment, &QPushButton::clicked, this, &MainWindow::onEditReservationCommentClicked);
    actionRow->addWidget(btnComment);

    QPushButton* btnReceipt = new QPushButton("Квитанция и предзаказ");
    connect(btnReceipt, &QPushButton::clicked, this, &MainWindow::onShowReceiptClicked);
    actionRow->addWidget(btnReceipt);

    listLayout->addLayout(actionRow);

    listLayout->addWidget(new QLabel("Лист ожидания (Waitlist):"));
    waitlistTable_ = new QTableWidget(0, 5);
    waitlistTable_->setHorizontalHeaderLabels({"ID", "Гость", "Телефон", "Желаемый час", "Комментарий"});
    waitlistTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    waitlistTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listLayout->addWidget(waitlistTable_, 1);

    layout->addWidget(listBox, 3);
    return tab;
}

QWidget* MainWindow::createRecommenderTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    // 1. Левая половина: Подбор идеального столика
    QGroupBox* tblBox = new QGroupBox("Умный подбор столика под ваш запрос");
    QVBoxLayout* tblLayout = new QVBoxLayout(tblBox);

    QFormLayout* tForm = new QFormLayout();
    recPartySpin_ = new QSpinBox();
    recPartySpin_->setRange(1, 10);
    recPartySpin_->setValue(2);
    tForm->addRow("Количество гостей:", recPartySpin_);

    recOccasionCombo_ = new QComboBox();
    recOccasionCombo_->addItems({"Свидание", "Деловая встреча", "День рождения", "Семейный ужин", "Обычный визит"});
    tForm->addRow("Повод визита:", recOccasionCombo_);
    tblLayout->addLayout(tForm);

    recWindowCheck_ = new QCheckBox("Хочу столик у панорамного окна");
    recWindowCheck_->setChecked(true);
    recQuietCheck_ = new QCheckBox("Тихая уединенная зона");
    recQuietCheck_->setChecked(true);
    recOutletCheck_ = new QCheckBox("Нужна розетка 220V / USB");
    recSofaCheck_ = new QCheckBox("Мягкие диваны");

    tblLayout->addWidget(recWindowCheck_);
    tblLayout->addWidget(recQuietCheck_);
    tblLayout->addWidget(recOutletCheck_);
    tblLayout->addWidget(recSofaCheck_);

    QPushButton* runTblRecBtn = new QPushButton("Найти лучшие свободные столики");
    runTblRecBtn->setStyleSheet("background-color: #D4AF37; color: #181A24; font-weight: bold; padding: 8px;");
    connect(runTblRecBtn, &QPushButton::clicked, this, &MainWindow::onRunTableRecommender);
    tblLayout->addWidget(runTblRecBtn);

    recTablesWidget_ = new QTableWidget(0, 5);
    recTablesWidget_->setHorizontalHeaderLabels({"Столик", "Зона", "Мест", "Совпадение", "Почему подходит"});
    recTablesWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    recTablesWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    recTablesWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(recTablesWidget_, &QTableWidget::cellDoubleClicked, this, &MainWindow::onSelectRecommendedTable);
    tblLayout->addWidget(recTablesWidget_, 1);

    tblLayout->addWidget(new QLabel("Совет: дважды кликните по строке рекомендованного стола, чтобы перейти к его бронированию."));

    // 2. Правая половина: Гастрономический сомелье (подбор сета блюд)
    QGroupBox* menuBox = new QGroupBox("Персональный подбор гастрономического сета по меню");
    QVBoxLayout* menuLayout = new QVBoxLayout(menuBox);

    QFormLayout* mForm = new QFormLayout();
    recBudgetSpin_ = new QDoubleSpinBox();
    recBudgetSpin_->setRange(500.0, 25000.0);
    recBudgetSpin_->setSingleStep(100.0);
    recBudgetSpin_->setValue(2600.0);
    recBudgetSpin_->setSuffix(" руб.");
    mForm->addRow("Бюджет на 1 гостя:", recBudgetSpin_);

    recDietCombo_ = new QComboBox();
    recDietCombo_->addItems({"Все", "вегетарианское", "без глютена", "морепродукты", "легкое", "хит шефа"});
    mForm->addRow("Диетическое предпочтение:", recDietCombo_);

    recMaxCalSpin_ = new QSpinBox();
    recMaxCalSpin_->setRange(0, 5000);
    recMaxCalSpin_->setSingleStep(100);
    recMaxCalSpin_->setValue(0);
    recMaxCalSpin_->setSpecialValueText("Без лимита ккал");
    mForm->addRow("Лимит калорий сета:", recMaxCalSpin_);

    menuLayout->addLayout(mForm);

    recNoSpicyCheck_ = new QCheckBox("Исключить острые блюда");
    recNoSpicyCheck_->setChecked(true);
    menuLayout->addWidget(recNoSpicyCheck_);

    QPushButton* runMenuRecBtn = new QPushButton("Сформировать персональные сеты блюд");
    runMenuRecBtn->setStyleSheet("background-color: #2980B9; color: white; font-weight: bold; padding: 8px;");
    connect(runMenuRecBtn, &QPushButton::clicked, this, &MainWindow::onRunMenuRecommender);
    menuLayout->addWidget(runMenuRecBtn);

    recMenuSetsList_ = new QListWidget();
    menuLayout->addWidget(recMenuSetsList_, 1);

    QPushButton* applySetBtn = new QPushButton("Перенести выбранный сет в корзину предзаказа");
    applySetBtn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold; padding: 8px;");
    connect(applySetBtn, &QPushButton::clicked, this, &MainWindow::onApplyRecommendedMenuSet);
    menuLayout->addWidget(applySetBtn);

    layout->addWidget(tblBox, 1);
    layout->addWidget(menuBox, 1);
    return tab;
}

QWidget* MainWindow::createMenuTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* filterBox = new QGroupBox("Фильтрация и поиск по меню ресторана");
    QHBoxLayout* fLayout = new QHBoxLayout(filterBox);

    fLayout->addWidget(new QLabel("Категория:"));
    menuCategoryFilter_ = new QComboBox();
    menuCategoryFilter_->addItems({"Все категории", "Закуски", "Супы", "Горячее", "Десерты", "Напитки"});
    fLayout->addWidget(menuCategoryFilter_);

    fLayout->addWidget(new QLabel("Тег:"));
    menuTagFilter_ = new QComboBox();
    menuTagFilter_->addItems({"Все теги", "хит шефа", "вегетарианское", "без глютена", "морепродукты", "легкое", "острое"});
    fLayout->addWidget(menuTagFilter_);

    fLayout->addWidget(new QLabel("Поиск:"));
    menuSearchEdit_ = new QLineEdit();
    menuSearchEdit_->setPlaceholderText("Название или ингредиент...");
    fLayout->addWidget(menuSearchEdit_, 1);

    fLayout->addWidget(new QLabel("Макс. цена:"));
    menuMaxPriceSpin_ = new QDoubleSpinBox();
    menuMaxPriceSpin_->setRange(0.0, 15000.0);
    menuMaxPriceSpin_->setSingleStep(100.0);
    menuMaxPriceSpin_->setValue(0.0);
    menuMaxPriceSpin_->setSpecialValueText("Любая");
    fLayout->addWidget(menuMaxPriceSpin_);

    QPushButton* addNewDishBtn = new QPushButton("+ Добавить блюдо в SQLite");
    connect(addNewDishBtn, &QPushButton::clicked, this, &MainWindow::onAddNewDishClicked);
    fLayout->addWidget(addNewDishBtn);

    connect(menuCategoryFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterMenuChanged);
    connect(menuTagFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterMenuChanged);
    connect(menuSearchEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterMenuChanged);
    connect(menuMaxPriceSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onFilterMenuChanged);

    layout->addWidget(filterBox);

    menuTableWidget_ = new QTableWidget(0, 8);
    menuTableWidget_->setHorizontalHeaderLabels(
        {"ID", "Блюдо", "Категория", "Цена", "Вес и КБЖУ", "Подача", "Рейтинг ★", "Теги и состав"});
    menuTableWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    menuTableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    menuTableWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(menuTableWidget_, 1);

    return tab;
}

QWidget* MainWindow::createReviewsAndAnalyticsTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    // Левая часть: Отзывы и комментарии гостей
    QGroupBox* revBox = new QGroupBox("Комментарии, оценки столов/блюд и ответы управляющего");
    QVBoxLayout* revLayout = new QVBoxLayout(revBox);

    reviewsTable_ = new QTableWidget(0, 6);
    reviewsTable_->setHorizontalHeaderLabels({"ID", "Гость и дата", "Оценка", "Стол / Блюдо", "Комментарий", "Ответ ресторана"});
    reviewsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    reviewsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    reviewsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    revLayout->addWidget(reviewsTable_, 1);

    QGroupBox* addRevBox = new QGroupBox("Оставить новый отзыв");
    QGridLayout* addGrid = new QGridLayout(addRevBox);

    addGrid->addWidget(new QLabel("Имя гостя:"), 0, 0);
    revAuthorEdit_ = new QLineEdit("Гость");
    addGrid->addWidget(revAuthorEdit_, 0, 1);

    addGrid->addWidget(new QLabel("Оценка (1..5 ★):"), 0, 2);
    revRatingSpin_ = new QSpinBox();
    revRatingSpin_->setRange(1, 5);
    revRatingSpin_->setValue(5);
    addGrid->addWidget(revRatingSpin_, 0, 3);

    addGrid->addWidget(new QLabel("Столик:"), 1, 0);
    revTableCombo_ = new QComboBox();
    revTableCombo_->addItem("Не привязан", 0);
    for (const auto& t : db_.getAllTables()) {
        revTableCombo_->addItem(QString("Стол #%1 [%2]").arg(t.id).arg(t.code), t.id);
    }
    addGrid->addWidget(revTableCombo_, 1, 1);

    addGrid->addWidget(new QLabel("Блюдо:"), 1, 2);
    revDishCombo_ = new QComboBox();
    revDishCombo_->addItem("Не привязано", 0);
    for (const auto& d : db_.getAllDishes()) {
        revDishCombo_->addItem(d.name, d.id);
    }
    addGrid->addWidget(revDishCombo_, 1, 3);

    addGrid->addWidget(new QLabel("Текст отзыва:"), 2, 0);
    revCommentEdit_ = new QLineEdit();
    revCommentEdit_->setPlaceholderText("Поделитесь впечатлениями о столике, кухне и атмосфере...");
    addGrid->addWidget(revCommentEdit_, 2, 1, 1, 3);

    QPushButton* postRevBtn = new QPushButton("Опубликовать отзыв");
    postRevBtn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold;");
    connect(postRevBtn, &QPushButton::clicked, this, &MainWindow::onAddReviewClicked);
    addGrid->addWidget(postRevBtn, 3, 0, 1, 2);

    QPushButton* replyBtn = new QPushButton("Ответить от имени ресторана на выбранный отзыв");
    connect(replyBtn, &QPushButton::clicked, this, &MainWindow::onReplyToReviewClicked);
    addGrid->addWidget(replyBtn, 3, 2, 1, 2);

    revLayout->addWidget(addRevBox);

    // Правая часть: Аналитическая панель управляющего
    QGroupBox* statBox = new QGroupBox("Аналитика загрузки зала и выручки (SQLite)");
    QVBoxLayout* statLayout = new QVBoxLayout(statBox);

    analyticsSummaryLabel_ = new QLabel();
    analyticsSummaryLabel_->setWordWrap(true);
    analyticsSummaryLabel_->setStyleSheet("font-size: 13px; background-color: #181A24; padding: 12px; border-radius: 6px;");
    statLayout->addWidget(analyticsSummaryLabel_);

    statLayout->addWidget(new QLabel("Почасовая загрузка столов (12:00 – 22:00):"));
    QGridLayout* barsGrid = new QGridLayout();
    hourlyBars_.clear();
    for (int h = 12; h <= 22; ++h) {
        int row = h - 12;
        barsGrid->addWidget(new QLabel(QString("%1:00").arg(h, 2, 10, QChar('0'))), row, 0);
        QProgressBar* bar = new QProgressBar();
        bar->setRange(0, 100);
        bar->setFormat("%p% столов занято");
        barsGrid->addWidget(bar, row, 1);
        hourlyBars_.push_back(bar);
    }
    statLayout->addLayout(barsGrid);
    statLayout->addStretch(1);

    layout->addWidget(revBox, 3);
    layout->addWidget(statBox, 2);
    return tab;
}

void MainWindow::onDateTimeFilterChanged() {
    refreshAllViews();
}

void MainWindow::refreshAllViews() {
    refreshFloorMapAndCard();
    refreshReservationsAndWaitlist();
    refreshPreorderWidget();
    refreshMenuTable();
    refreshReviewsAndAnalytics();
}

void MainWindow::refreshFloorMapAndCard() {
    auto tables = db_.getAllTables();
    QString date = activeDateString();
    int hour = hourSpin_->value();
    int dur = durationSpin_->value();

    QMap<int, int> occMap;
    QMap<int, double> ratMap;
    int freeCount = 0;
    int seatedCount = 0;

    for (const auto& t : tables) {
        int st = db_.getTableOccupancyStatus(t.id, date, hour, dur);
        occMap[t.id] = st;
        ratMap[t.id] = db_.getAverageTableRating(t.id);
        if (st == 0) ++freeCount;
        if (st == 2) ++seatedCount;
    }

    floorMapWidget_->updateFloorData(tables, occMap, ratMap);

    summaryBannerLabel_->setText(
        QString("Свободно столов: %1 из %2   |   Сейчас за столами: %3   |   Средний рейтинг ресторана: ★ %4 / 5.0")
        .arg(freeCount)
        .arg(tables.size())
        .arg(seatedCount)
        .arg(db_.getOverallRating(), 0, 'f', 2));

    onTableSelectedOnMap(currentTableId_);
}

void MainWindow::onTableSelectedOnMap(int tableId) {
    currentTableId_ = tableId;
    TableInfo t = db_.getTableById(tableId);
    if (t.id == 0) return;

    cardTitleLabel_->setText(QString("Столик #%1 [%2] — до %3 персон").arg(t.id).arg(t.code).arg(t.capacity));
    cardZoneLabel_->setText(QString("<b>Зона:</b> %1 (координаты X=%2, Y=%3)<br><b>Описание:</b> %4")
                            .arg(t.zone).arg(t.gridX).arg(t.gridY).arg(t.description));

    QStringList feats;
    if (t.nearWindow) feats << "Панорамное окно";
    if (t.hasSofa)    feats << "Мягкие диваны";
    if (t.hasOutlet)  feats << "Розетка 220V/USB";
    if (t.isQuiet)    feats << "Тихая уединенная зона";
    if (feats.isEmpty()) feats << "Стандартная комплектация";
    cardFeaturesLabel_->setText("<b>Оснащение:</b> " + feats.join(", "));

    cardDepositLabel_->setText(t.minDeposit > 0.0
        ? QString("<b>Минимальный депозит:</b> %1 руб.").arg(t.minDeposit, 0, 'f', 0)
        : "<b>Депозит:</b> Без обязательного депозита");

    double avgR = db_.getAverageTableRating(t.id);
    cardRatingLabel_->setText(avgR > 0.0
        ? QString("Рейтинг стола по отзывам: ★ %1 / 5.0").arg(avgR, 0, 'f', 1)
        : "Рейтинг стола: пока нет оценок");

    QString date = activeDateString();
    QString sched = QString("Почасовой график на %1:\n").arg(date);
    for (int h = 10; h < 23; ++h) {
        int st = db_.getTableOccupancyStatus(t.id, date, h, 1);
        QString tag = (st == 0) ? "[Своб]" : (st == 1 ? "[Бронь]" : "[Занят]");
        sched += QString("%1:00%2 ").arg(h, 2, 10, QChar('0')).arg(tag);
        if (h == 15) sched += "\n";
    }
    cardScheduleLabel_->setText(sched);

    cardReviewsList_->clear();
    for (const auto& rev : db_.getAllReviews()) {
        if (rev.tableId == t.id) {
            cardReviewsList_->addItem(QString("★ %1/5 — %2 (%3): «%4»")
                                      .arg(rev.rating).arg(rev.authorName).arg(rev.reviewDate).arg(rev.comment));
        }
    }
    if (cardReviewsList_->count() == 0) {
        cardReviewsList_->addItem("Пока нет отзывов об этом столике.");
    }
}

void MainWindow::onQuickBookFromMap() {
    int idx = tableSelectCombo_->findData(currentTableId_);
    if (idx >= 0) tableSelectCombo_->setCurrentIndex(idx);
    bookingTypeCombo_->setCurrentIndex(0); // Online
    mainTabs_->setCurrentIndex(1);
}

void MainWindow::onQuickWalkInFromMap() {
    int idx = tableSelectCombo_->findData(currentTableId_);
    if (idx >= 0) tableSelectCombo_->setCurrentIndex(idx);
    bookingTypeCombo_->setCurrentIndex(1); // WalkIn
    mainTabs_->setCurrentIndex(1);
}

void MainWindow::onAddDishToPreorder() {
    int dishId = preorderDishCombo_->currentData().toInt();
    int qty = preorderQtySpin_->value();
    DishInfo d = db_.getDishById(dishId);
    if (d.id == 0) return;

    for (auto& item : currentPreorder_) {
        if (item.dishId == dishId) {
            item.quantity += qty;
            refreshPreorderWidget();
            return;
        }
    }
    currentPreorder_.push_back({d.id, d.name, d.price, qty});
    refreshPreorderWidget();
}

void MainWindow::onClearPreorder() {
    currentPreorder_.clear();
    refreshPreorderWidget();
}

void MainWindow::refreshPreorderWidget() {
    preorderListWidget_->clear();
    for (const auto& item : currentPreorder_) {
        preorderListWidget_->addItem(
            QString("%1 x%2 = %3 руб.").arg(item.dishName).arg(item.quantity).arg(item.unitPrice * item.quantity, 0, 'f', 0));
    }
    bool isOnline = (bookingTypeCombo_->currentData().toString() == "Online");
    double total = db_.calculatePreorderTotal(currentPreorder_, isOnline);
    if (isOnline && !currentPreorder_.empty()) {
        preorderTotalLabel_->setText(QString("Итого со скидкой 5% (онлайн-предзаказ): %1 руб.").arg(total, 0, 'f', 0));
    } else {
        preorderTotalLabel_->setText(QString("Итого по предзаказу: %1 руб.").arg(total, 0, 'f', 0));
    }
}

void MainWindow::onCreateReservationClicked() {
    ReservationInfo res;
    res.guestName = guestNameEdit_->text();
    res.guestPhone = guestPhoneEdit_->text();
    res.visitDate = activeDateString();
    res.startHour = hourSpin_->value();
    res.durationHours = durationSpin_->value();
    res.partySize = partySizeSpin_->value();
    res.tableId = tableSelectCombo_->currentData().toInt();
    res.bookingType = bookingTypeCombo_->currentData().toString();
    res.occasion = occasionCombo_->currentText();
    res.comment = bookingCommentEdit_->toPlainText().trimmed();
    res.preorder = currentPreorder_;

    QString code;
    int newId = db_.createReservation(res, code);
    if (newId == 0) {
        QMessageBox::warning(this, "Столик недоступен",
            "Выбранный столик уже занят на это время или его вместимость меньше размера вашей компании.\n"
            "Воспользуйтесь вкладкой «Рекомендации под твой запрос» или добавьте гостя в Лист ожидания.");
        return;
    }

    currentPreorder_.clear();
    bookingCommentEdit_->clear();
    refreshAllViews();

    QMessageBox::information(this, "Запись успешно создана",
        QString("Бронирование %1 успешно сохранено в базе SQLite!\nСтолик #%2 на %3 с %4:00.")
        .arg(code).arg(res.tableId).arg(res.visitDate).arg(res.startHour));
}

int MainWindow::selectedReservationIdFromTable() const {
    auto items = reservationsTable_->selectedItems();
    if (items.isEmpty()) return 0;
    int row = items.first()->row();
    return reservationsTable_->item(row, 0)->text().toInt();
}

void MainWindow::onCheckInClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) return;
    if (db_.updateReservationStatus(id, "Seated")) {
        refreshAllViews();
    }
}

void MainWindow::onCheckOutClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) return;
    if (db_.updateReservationStatus(id, "Completed")) {
        refreshAllViews();
    }
}

void MainWindow::onCancelReservationClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) return;
    if (db_.updateReservationStatus(id, "Cancelled")) {
        refreshAllViews();
    }
}

void MainWindow::onEditReservationCommentClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) return;
    ReservationInfo r = db_.getReservationById(id);
    bool ok = false;
    QString text = QInputDialog::getText(this, "Комментарий к брони " + r.bookingCode,
                                         "Пожелания гостя / комментарий хостес:", QLineEdit::Normal, r.comment, &ok);
    if (ok) {
        db_.updateReservationComment(id, text);
        refreshAllViews();
    }
}

void MainWindow::onShowReceiptClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) return;
    ReservationInfo r = db_.getReservationById(id);
    TableInfo t = db_.getTableById(r.tableId);

    QString receipt = QString(
        "КВИТАНЦИЯ БРОНИРОВАНИЯ %1\n"
        "----------------------------------------\n"
        "Гость: %2 (%3)\n"
        "Дата и время: %4, %5:00 - %6:00\n"
        "Столик: #%7 [%8] (%9)\n"
        "Персон: %10 | Повод: %11\n"
        "Тип: %12 | Статус: %13\n"
        "Комментарий: %14\n")
        .arg(r.bookingCode).arg(r.guestName).arg(r.guestPhone)
        .arg(r.visitDate).arg(r.startHour).arg(r.startHour + r.durationHours)
        .arg(t.id).arg(t.code).arg(t.zone)
        .arg(r.partySize).arg(r.occasion).arg(r.bookingType).arg(r.status)
        .arg(r.comment.isEmpty() ? "—" : r.comment);

    if (!r.preorder.empty()) {
        receipt += "\nПредзаказ по меню:\n";
        for (const auto& item : r.preorder) {
            receipt += QString(" • %1 x%2 = %3 руб.\n")
                       .arg(item.dishName).arg(item.quantity).arg(item.unitPrice * item.quantity, 0, 'f', 0);
        }
        bool isOnline = (r.bookingType == "Online");
        double total = db_.calculatePreorderTotal(r.preorder, isOnline);
        receipt += QString("ИТОГО%1: %2 руб.")
                   .arg(isOnline ? " (со скидкой 5% онлайн)" : "")
                   .arg(total, 0, 'f', 0);
    }

    QMessageBox::information(this, "Детали бронирования " + r.bookingCode, receipt);
}

void MainWindow::onAddWaitlistClicked() {
    WaitlistInfo w;
    w.guestName = guestNameEdit_->text();
    w.guestPhone = guestPhoneEdit_->text();
    w.visitDate = activeDateString();
    w.preferredHour = hourSpin_->value();
    w.partySize = partySizeSpin_->value();
    w.comment = bookingCommentEdit_->toPlainText().trimmed();
    if (w.comment.isEmpty()) w.comment = "Ожидает освобождения столика";

    if (db_.addToWaitlist(w)) {
        refreshReservationsAndWaitlist();
        QMessageBox::information(this, "Лист ожидания", "Гость успешно добавлен в Лист ожидания (Waitlist).");
    }
}

void MainWindow::refreshReservationsAndWaitlist() {
    auto list = db_.getReservationsByDate(activeDateString());
    reservationsTable_->setRowCount(static_cast<int>(list.size()));
    for (size_t i = 0; i < list.size(); ++i) {
        const auto& r = list[i];
        TableInfo t = db_.getTableById(r.tableId);
        int row = static_cast<int>(i);
        reservationsTable_->setItem(row, 0, new QTableWidgetItem(QString::number(r.id)));
        reservationsTable_->setItem(row, 1, new QTableWidgetItem(r.bookingCode));
        reservationsTable_->setItem(row, 2, new QTableWidgetItem(
            QString("%1:00-%2:00").arg(r.startHour, 2, 10, QChar('0')).arg(r.startHour + r.durationHours, 2, 10, QChar('0'))));
        reservationsTable_->setItem(row, 3, new QTableWidgetItem(QString("#%1 [%2]").arg(t.id).arg(t.code)));
        reservationsTable_->setItem(row, 4, new QTableWidgetItem(r.guestName));
        reservationsTable_->setItem(row, 5, new QTableWidgetItem(QString::number(r.partySize)));
        reservationsTable_->setItem(row, 6, new QTableWidgetItem(r.bookingType == "Online" ? "Онлайн" : "Walk-in"));
        reservationsTable_->setItem(row, 7, new QTableWidgetItem(r.status));
    }

    auto wl = db_.getWaitlist();
    waitlistTable_->setRowCount(static_cast<int>(wl.size()));
    for (size_t i = 0; i < wl.size(); ++i) {
        int row = static_cast<int>(i);
        waitlistTable_->setItem(row, 0, new QTableWidgetItem(QString::number(wl[i].id)));
        waitlistTable_->setItem(row, 1, new QTableWidgetItem(wl[i].guestName));
        waitlistTable_->setItem(row, 2, new QTableWidgetItem(wl[i].guestPhone));
        waitlistTable_->setItem(row, 3, new QTableWidgetItem(QString("%1 (%2:00)").arg(wl[i].visitDate).arg(wl[i].preferredHour)));
        waitlistTable_->setItem(row, 4, new QTableWidgetItem(wl[i].comment));
    }
}

void MainWindow::onRunTableRecommender() {
    TablePreference pref;
    pref.partySize = recPartySpin_->value();
    pref.visitDate = activeDateString();
    pref.startHour = hourSpin_->value();
    pref.durationHours = durationSpin_->value();
    pref.occasion = recOccasionCombo_->currentText();
    pref.preferWindow = recWindowCheck_->isChecked();
    pref.preferQuiet = recQuietCheck_->isChecked();
    pref.needOutlet = recOutletCheck_->isChecked();
    pref.preferSofa = recSofaCheck_->isChecked();

    lastTableRecs_ = db_.recommendTables(pref);
    recTablesWidget_->setRowCount(static_cast<int>(lastTableRecs_.size()));
    for (size_t i = 0; i < lastTableRecs_.size(); ++i) {
        const auto& r = lastTableRecs_[i];
        int row = static_cast<int>(i);
        recTablesWidget_->setItem(row, 0, new QTableWidgetItem(QString("#%1 [%2]").arg(r.table.id).arg(r.table.code)));
        recTablesWidget_->setItem(row, 1, new QTableWidgetItem(r.table.zone));
        recTablesWidget_->setItem(row, 2, new QTableWidgetItem(QString::number(r.table.capacity)));
        recTablesWidget_->setItem(row, 3, new QTableWidgetItem(QString("%1 / 100").arg(r.matchScore)));
        recTablesWidget_->setItem(row, 4, new QTableWidgetItem(r.reasons.join("; ")));
    }
}

void MainWindow::onSelectRecommendedTable(int row, int /*col*/) {
    if (row < 0 || row >= static_cast<int>(lastTableRecs_.size())) return;
    int tblId = lastTableRecs_[row].table.id;
    floorMapWidget_->setSelectedTableId(tblId);
    int idx = tableSelectCombo_->findData(tblId);
    if (idx >= 0) tableSelectCombo_->setCurrentIndex(idx);
    partySizeSpin_->setValue(recPartySpin_->value());
    occasionCombo_->setCurrentText(recOccasionCombo_->currentText());
    mainTabs_->setCurrentIndex(1);
}

void MainWindow::onRunMenuRecommender() {
    MenuPreference pref;
    pref.maxBudget = recBudgetSpin_->value();
    pref.dietaryTag = recDietCombo_->currentText();
    pref.avoidSpicy = recNoSpicyCheck_->isChecked();
    pref.maxCalories = recMaxCalSpin_->value();

    lastMenuSets_ = db_.recommendMenuSets(pref);
    recMenuSetsList_->clear();
    for (const auto& s : lastMenuSets_) {
        QString text = QString("%1 — %2 руб. (%3 ккал)\n   %4\n")
                       .arg(s.title).arg(s.totalPrice, 0, 'f', 0).arg(s.totalCalories).arg(s.explanation);
        for (const auto& d : s.dishes) {
            text += QString("   • [%1] %2 — %3 руб. (%4 ккал)\n")
                    .arg(d.category).arg(d.name).arg(d.price, 0, 'f', 0).arg(d.calories);
        }
        recMenuSetsList_->addItem(text.trimmed());
    }
}

void MainWindow::onApplyRecommendedMenuSet() {
    int row = recMenuSetsList_->currentRow();
    if (row < 0 || row >= static_cast<int>(lastMenuSets_.size())) {
        row = 0;
    }
    if (lastMenuSets_.empty()) return;

    currentPreorder_.clear();
    for (const auto& d : lastMenuSets_[row].dishes) {
        currentPreorder_.push_back({d.id, d.name, d.price, 1});
    }
    refreshPreorderWidget();
    mainTabs_->setCurrentIndex(1);
}

void MainWindow::onFilterMenuChanged() {
    refreshMenuTable();
}

void MainWindow::refreshMenuTable() {
    auto dishes = db_.searchDishes(
        menuCategoryFilter_->currentText(),
        menuTagFilter_->currentText(),
        menuSearchEdit_->text(),
        menuMaxPriceSpin_->value());

    menuTableWidget_->setRowCount(static_cast<int>(dishes.size()));
    for (size_t i = 0; i < dishes.size(); ++i) {
        const auto& d = dishes[i];
        int row = static_cast<int>(i);
        double avgR = db_.getAverageDishRating(d.id);
        menuTableWidget_->setItem(row, 0, new QTableWidgetItem(QString::number(d.id)));
        menuTableWidget_->setItem(row, 1, new QTableWidgetItem(d.name));
        menuTableWidget_->setItem(row, 2, new QTableWidgetItem(d.category));
        menuTableWidget_->setItem(row, 3, new QTableWidgetItem(QString("%1 руб.").arg(d.price, 0, 'f', 0)));
        menuTableWidget_->setItem(row, 4, new QTableWidgetItem(QString("%1 г | %2 ккал").arg(d.weightGrams).arg(d.calories)));
        menuTableWidget_->setItem(row, 5, new QTableWidgetItem(QString("~%1 мин").arg(d.prepTimeMin)));
        menuTableWidget_->setItem(row, 6, new QTableWidgetItem(avgR > 0.0 ? QString("★ %1").arg(avgR, 0, 'f', 1) : "Новинка"));
        menuTableWidget_->setItem(row, 7, new QTableWidgetItem(QString("#%1 — %2").arg(d.tags.join(", #")).arg(d.description)));
    }
}

void MainWindow::onAddNewDishClicked() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "Новое блюдо", "Название блюда:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString cat = QInputDialog::getItem(this, "Категория", "Выберите категорию:",
                                        {"Закуски", "Супы", "Горячее", "Десерты", "Напитки"}, 0, false, &ok);
    if (!ok) return;

    double price = QInputDialog::getDouble(this, "Цена", "Цена (руб.):", 950.0, 100.0, 20000.0, 0, &ok);
    if (!ok) return;

    DishInfo d;
    d.name = name.trimmed();
    d.category = cat;
    d.price = price;
    d.weightGrams = 250;
    d.calories = 380;
    d.prepTimeMin = 15;
    d.tags = {"хит шефа"};
    d.description = "Авторское блюдо шеф-повара";

    if (db_.addDish(d)) {
        preorderDishCombo_->addItem(QString("%1 (%2 руб.)").arg(d.name).arg(d.price, 0, 'f', 0));
        refreshMenuTable();
    }
}

void MainWindow::onAddReviewClicked() {
    ReviewInfo r;
    r.authorName = revAuthorEdit_->text();
    r.reviewDate = activeDateString();
    r.rating = revRatingSpin_->value();
    r.tableId = revTableCombo_->currentData().toInt();
    r.dishId = revDishCombo_->currentData().toInt();
    r.comment = revCommentEdit_->text().trimmed();
    if (r.comment.isEmpty()) {
        QMessageBox::warning(this, "Пустой отзыв", "Пожалуйста, напишите текст вашего комментария.");
        return;
    }
    if (db_.addReview(r)) {
        revCommentEdit_->clear();
        refreshAllViews();
    }
}

void MainWindow::onReplyToReviewClicked() {
    auto items = reviewsTable_->selectedItems();
    if (items.isEmpty()) return;
    int revId = reviewsTable_->item(items.first()->row(), 0)->text().toInt();
    bool ok = false;
    QString reply = QInputDialog::getText(this, "Ответ ресторана", "Введите ответ управляющего на отзыв:",
                                          QLineEdit::Normal, "Благодарим за ваш отзыв! Ждем вас снова.", &ok);
    if (ok && !reply.trimmed().isEmpty()) {
        db_.addManagerReply(revId, reply.trimmed());
        refreshReviewsAndAnalytics();
    }
}

void MainWindow::refreshReviewsAndAnalytics() {
    auto revs = db_.getAllReviews();
    reviewsTable_->setRowCount(static_cast<int>(revs.size()));
    for (size_t i = 0; i < revs.size(); ++i) {
        const auto& r = revs[i];
        int row = static_cast<int>(i);
        reviewsTable_->setItem(row, 0, new QTableWidgetItem(QString::number(r.id)));
        reviewsTable_->setItem(row, 1, new QTableWidgetItem(QString("%1 (%2)").arg(r.authorName).arg(r.reviewDate)));
        reviewsTable_->setItem(row, 2, new QTableWidgetItem(QString("★ %1 / 5").arg(r.rating)));

        QString target;
        if (r.tableId > 0) target += QString("Стол #%1 ").arg(r.tableId);
        if (r.dishId > 0)  target += QString("Блюдо #%1").arg(r.dishId);
        reviewsTable_->setItem(row, 3, new QTableWidgetItem(target.isEmpty() ? "Общий" : target));
        reviewsTable_->setItem(row, 4, new QTableWidgetItem(r.comment));
        reviewsTable_->setItem(row, 5, new QTableWidgetItem(r.managerReply.isEmpty() ? "—" : r.managerReply));
    }

    // Расчет аналитики по базе SQLite на выбранную дату
    QString date = activeDateString();
    auto dayBookings = db_.getReservationsByDate(date);
    int onlineCnt = 0, walkInCnt = 0, totalGuests = 0;
    double revenue = 0.0;

    for (const auto& b : dayBookings) {
        if (b.status == "Cancelled") continue;
        if (b.bookingType == "Online") ++onlineCnt;
        else ++walkInCnt;
        totalGuests += b.partySize;
        double pre = db_.calculatePreorderTotal(b.preorder, b.bookingType == "Online");
        TableInfo t = db_.getTableById(b.tableId);
        revenue += std::max(pre, t.minDeposit);
    }

    analyticsSummaryLabel_->setText(
        QString("<b>Сводка за %1:</b><br>"
                "• Активных записей: <b>%2</b> (Онлайн: %3 | В ресторане Walk-in: %4)<br>"
                "• Ожидаемое число гостей: <b>%5 чел.</b><br>"
                "• Выручка по предзаказам и депозитам: <b>%6 руб.</b><br>"
                "• Средний рейтинг по базе SQLite: <b>★ %7 / 5.0</b> (отзывов: %8)")
        .arg(date)
        .arg(onlineCnt + walkInCnt).arg(onlineCnt).arg(walkInCnt)
        .arg(totalGuests)
        .arg(revenue, 0, 'f', 0)
        .arg(db_.getOverallRating(), 0, 'f', 2)
        .arg(revs.size()));

    auto allTables = db_.getAllTables();
    int totalTbls = std::max(1, static_cast<int>(allTables.size()));
    for (int h = 12; h <= 22; ++h) {
        int busy = 0;
        for (const auto& t : allTables) {
            if (db_.getTableOccupancyStatus(t.id, date, h, 1) != 0) ++busy;
        }
        int idx = h - 12;
        if (idx >= 0 && idx < static_cast<int>(hourlyBars_.size())) {
            hourlyBars_[idx]->setValue(busy * 100 / totalTbls);
        }
    }
}

} // namespace Restaurant
