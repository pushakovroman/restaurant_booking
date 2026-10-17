#include "MainWindow.hpp"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QGraphicsDropShadowEffect>
#include <algorithm>

namespace Restaurant {

QString MainWindow::darkLuxuryStyleSheet() {
    return R"(
        QMainWindow, QWidget {
            background-color: #171923;
            color: #ECEFF4;
            font-family: "Segoe UI", "Noto Sans", sans-serif;
            font-size: 12px;
        }
        QFrame#SidebarFrame {
            background-color: #11131C;
            border-right: 1px solid #2D3246;
        }
        QFrame#KpiCard {
            background-color: #212434;
            border: 1px solid #353B54;
            border-radius: 10px;
        }
        QListWidget#SidebarNav {
            background-color: transparent;
            border: none;
            outline: none;
            font-size: 13px;
            font-weight: bold;
        }
        QListWidget#SidebarNav::item {
            padding: 12px 14px;
            margin: 4px 8px;
            border-radius: 8px;
            color: #B8C1D1;
        }
        QListWidget#SidebarNav::item:hover {
            background-color: #232738;
            color: #FFFFFF;
        }
        QListWidget#SidebarNav::item:selected {
            background-color: #D4AF37;
            color: #14161F;
        }
        QGroupBox {
            background-color: #1E2130;
            border: 1px solid #32384F;
            border-radius: 10px;
            margin-top: 12px;
            padding-top: 12px;
            font-weight: bold;
            color: #F5D76E;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 14px;
            padding: 0 6px;
        }
        QLineEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit {
            background-color: #141620;
            border: 1px solid #3E4560;
            border-radius: 6px;
            padding: 6px 8px;
            color: #FFFFFF;
        }
        QTableWidget, QListWidget {
            background-color: #161823;
            border: 1px solid #2F354B;
            border-radius: 8px;
            gridline-color: #272B3D;
            selection-background-color: #364F6B;
        }
        QHeaderView::section {
            background-color: #25293C;
            color: #F5D76E;
            padding: 7px;
            border: 1px solid #32384F;
            font-weight: bold;
        }
        QPushButton {
            background-color: #2D334A;
            color: #FFFFFF;
            border: 1px solid #454E6E;
            border-radius: 7px;
            padding: 7px 13px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #3D4563;
        }
        QSlider::groove:horizontal {
            height: 8px;
            background: #272B3D;
            border-radius: 4px;
        }
        QSlider::sub-page:horizontal {
            background: #D4AF37;
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            background: #F5D76E;
            border: 2px solid #171923;
            width: 18px;
            margin: -6px 0;
            border-radius: 9px;
        }
    )";
}

QString MainWindow::lightCafeStyleSheet() {
    return R"(
        QMainWindow, QWidget {
            background-color: #F4F1EA;
            color: #2C3038;
            font-family: "Segoe UI", "Noto Sans", sans-serif;
            font-size: 12px;
        }
        QFrame#SidebarFrame {
            background-color: #E8E2D5;
            border-right: 1px solid #C8BFA8;
        }
        QFrame#KpiCard {
            background-color: #FFFFFF;
            border: 1px solid #D8D0C0;
            border-radius: 10px;
        }
        QListWidget#SidebarNav {
            background-color: transparent;
            border: none;
            outline: none;
            font-size: 13px;
            font-weight: bold;
        }
        QListWidget#SidebarNav::item {
            padding: 12px 14px;
            margin: 4px 8px;
            border-radius: 8px;
            color: #4A443B;
        }
        QListWidget#SidebarNav::item:hover {
            background-color: #DCD4C4;
        }
        QListWidget#SidebarNav::item:selected {
            background-color: #B8860B;
            color: #FFFFFF;
        }
        QGroupBox {
            background-color: #FFFFFF;
            border: 1px solid #D6CEBE;
            border-radius: 10px;
            margin-top: 12px;
            padding-top: 12px;
            font-weight: bold;
            color: #8B6508;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 14px;
            padding: 0 6px;
        }
        QLineEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit {
            background-color: #FAF8F5;
            border: 1px solid #C2B9A7;
            border-radius: 6px;
            padding: 6px 8px;
            color: #1E2028;
        }
        QTableWidget, QListWidget {
            background-color: #FFFFFF;
            border: 1px solid #D6CEBE;
            border-radius: 8px;
            gridline-color: #E6E0D4;
            selection-background-color: #D4AF37;
            selection-color: #181A24;
        }
        QHeaderView::section {
            background-color: #EAE4D7;
            color: #5C4308;
            padding: 7px;
            border: 1px solid #D6CEBE;
            font-weight: bold;
        }
        QPushButton {
            background-color: #E2DBD0;
            color: #2C3038;
            border: 1px solid #BFB6A4;
            border-radius: 7px;
            padding: 7px 13px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #D5CCB8;
        }
        QSlider::groove:horizontal {
            height: 8px;
            background: #D6CEBE;
            border-radius: 4px;
        }
        QSlider::sub-page:horizontal {
            background: #B8860B;
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            background: #8B6508;
            width: 18px;
            margin: -6px 0;
            border-radius: 9px;
        }
    )";
}

MainWindow::MainWindow(DatabaseManager& db, QWidget* parent)
    : QMainWindow(parent), db_(db) {
    setWindowTitle("Система бронирования «Aura Gastronomy Minsk» — Pro UI (Qt + SQLite • BYN)");
    resize(1420, 880);
    setupUi();
    reloadDishCombos();
    refreshAllViews();
    onRunTableRecommender();
    onRunMenuRecommender();
}

QString MainWindow::activeDateString() const {
    return dateEdit_->date().toString("yyyy-MM-dd");
}

void MainWindow::switchToPage(int index) {
    if (sidebarNav_ && pagesStack_ && index >= 0 && index < pagesStack_->count()) {
        sidebarNav_->setCurrentRow(index);
        pagesStack_->setCurrentIndex(index);
    }
}

QFrame* MainWindow::createKpiCard(const QString& title, QLabel*& outValueLabel, const QString& accentColor) {
    QFrame* card = new QFrame();
    card->setObjectName("KpiCard");
    QVBoxLayout* v = new QVBoxLayout(card);
    v->setContentsMargins(14, 10, 14, 10);
    v->setSpacing(4);

    QLabel* titleLbl = new QLabel(title);
    titleLbl->setStyleSheet("font-size: 11px; color: #9AA5B8; font-weight: bold;");
    v->addWidget(titleLbl);

    outValueLabel = new QLabel("—");
    outValueLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(accentColor));
    v->addWidget(outValueLabel);

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(16);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 90));
    card->setGraphicsEffect(shadow);

    return card;
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    QHBoxLayout* mainHLayout = new QHBoxLayout(central);
    mainHLayout->setContentsMargins(0, 0, 0, 0);
    mainHLayout->setSpacing(0);

    // 1. ЛЕВАЯ БОКОВАЯ ПАНЕЛЬ НАВИГАЦИИ (SIDEBAR)
    QFrame* sidebar = new QFrame();
    sidebar->setObjectName("SidebarFrame");
    sidebar->setFixedWidth(245);
    QVBoxLayout* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(10, 16, 10, 16);
    sideLayout->setSpacing(10);

    QLabel* brandTitle = new QLabel("AURA MINSK");
    brandTitle->setAlignment(Qt::AlignCenter);
    brandTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #D4AF37; letter-spacing: 2px;");
    sideLayout->addWidget(brandTitle);

    QLabel* brandSub = new QLabel("GASTRONOMY • РЕСТОРАН (BYN)");
    brandSub->setAlignment(Qt::AlignCenter);
    brandSub->setStyleSheet("font-size: 10px; color: #8892A6; margin-bottom: 10px;");
    sideLayout->addWidget(brandSub);

    sidebarNav_ = new QListWidget();
    sidebarNav_->setObjectName("SidebarNav");
    sidebarNav_->addItem("🗺   2D-Карта зала");
    sidebarNav_->addItem("📅   Запись и Хостес");
    sidebarNav_->addItem("✨   Рекомендации");
    sidebarNav_->addItem("🍽   Меню (BYN)");
    sidebarNav_->addItem("📊   Отзывы и Графики");
    sidebarNav_->setCurrentRow(0);
    sideLayout->addWidget(sidebarNav_, 1);

    themeToggleBtn_ = new QPushButton("☀ Светлая тема (Light Café)");
    connect(themeToggleBtn_, &QPushButton::clicked, this, &MainWindow::onToggleThemeClicked);
    sideLayout->addWidget(themeToggleBtn_);

    QPushButton* refreshSideBtn = new QPushButton("↻ Синхронизировать БД");
    connect(refreshSideBtn, &QPushButton::clicked, this, &MainWindow::refreshAllViews);
    sideLayout->addWidget(refreshSideBtn);

    mainHLayout->addWidget(sidebar);

    // 2. ПРАВАЯ РАБОЧАЯ ОБЛАСТЬ (KPI-шапка + Таймлайн + Стек экранов)
    QWidget* rightArea = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightArea);
    rightLayout->setContentsMargins(14, 12, 14, 12);
    rightLayout->setSpacing(10);

    // 2.1. Ряд из 4 KPI-карточек с тенями
    QHBoxLayout* kpiRow = new QHBoxLayout();
    kpiRow->setSpacing(12);
    kpiRow->addWidget(createKpiCard("🟢 СВОБОДНО СТОЛОВ", kpiFreeTablesLabel_, "#2ECC71"), 1);
    kpiRow->addWidget(createKpiCard("🔴 СЕЙЧАС В ЗАЛЕ", kpiSeatedGuestsLabel_, "#E74C3C"), 1);
    kpiRow->addWidget(createKpiCard("⏳ ЛИСТ ОЖИДАНИЯ", kpiWaitlistLabel_, "#F39C12"), 1);
    kpiRow->addWidget(createKpiCard("💳 ВЫРУЧКА ДНЯ И РЕЙТИНГ", kpiRevenueRatingLabel_, "#F5D76E"), 1);
    rightLayout->addLayout(kpiRow);

    // 2.2. Панель «Машина времени» с интерактивным ползунком часа (10:00 – 22:00)
    QGroupBox* timeBox = new QGroupBox("Интерактивный таймлайн зала («Машина времени» 10:00 – 23:00)");
    QHBoxLayout* timeLayout = new QHBoxLayout(timeBox);

    timeLayout->addWidget(new QLabel("Дата:"));
    dateEdit_ = new QDateEdit(QDate(2026, 10, 16));
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    timeLayout->addWidget(dateEdit_);

    timeLayout->addSpacing(10);
    timeLayout->addWidget(new QLabel("Час:"));
    hourSpin_ = new QSpinBox();
    hourSpin_->setRange(DatabaseManager::OPEN_HOUR, DatabaseManager::CLOSE_HOUR - 1);
    hourSpin_->setValue(19);
    hourSpin_->setSuffix(":00");
    timeLayout->addWidget(hourSpin_);

    timeLayout->addSpacing(8);
    timeLayout->addWidget(new QLabel("10:00"));
    timeSlider_ = new QSlider(Qt::Horizontal);
    timeSlider_->setRange(DatabaseManager::OPEN_HOUR, DatabaseManager::CLOSE_HOUR - 1);
    timeSlider_->setValue(19);
    timeSlider_->setTickPosition(QSlider::TicksBelow);
    timeSlider_->setTickInterval(1);
    timeLayout->addWidget(timeSlider_, 1);
    timeLayout->addWidget(new QLabel("22:00"));

    timeLayout->addSpacing(12);
    timeLayout->addWidget(new QLabel("Длительность визита:"));
    durationSpin_ = new QSpinBox();
    durationSpin_->setRange(1, 4);
    durationSpin_->setValue(2);
    durationSpin_->setSuffix(" ч.");
    timeLayout->addWidget(durationSpin_);

    connect(dateEdit_, &QDateEdit::dateChanged, this, &MainWindow::onDateTimeFilterChanged);
    connect(hourSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onDateTimeFilterChanged);
    connect(timeSlider_, &QSlider::valueChanged, this, &MainWindow::onTimeSliderChanged);
    connect(durationSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onDateTimeFilterChanged);

    rightLayout->addWidget(timeBox);

    // 2.3. Стек страниц
    pagesStack_ = new QStackedWidget();
    pagesStack_->addWidget(createFloorMapPage());
    pagesStack_->addWidget(createBookingPage());
    pagesStack_->addWidget(createRecommenderPage());
    pagesStack_->addWidget(createMenuPage());
    pagesStack_->addWidget(createReviewsAndAnalyticsPage());

    connect(sidebarNav_, &QListWidget::currentRowChanged, pagesStack_, &QStackedWidget::setCurrentIndex);

    rightLayout->addWidget(pagesStack_, 1);
    mainHLayout->addWidget(rightArea, 1);

    setCentralWidget(central);
}

void MainWindow::onToggleThemeClicked() {
    isDarkTheme_ = !isDarkTheme_;
    if (isDarkTheme_) {
        qApp->setStyleSheet(darkLuxuryStyleSheet());
        themeToggleBtn_->setText("☀ Светлая тема (Light Café)");
    } else {
        qApp->setStyleSheet(lightCafeStyleSheet());
        themeToggleBtn_->setText("🌙 Темная тема (Dark Luxury)");
    }
}

void MainWindow::onTimeSliderChanged(int value) {
    if (hourSpin_->value() != value) {
        hourSpin_->setValue(value);
    }
}

QWidget* MainWindow::createFloorMapPage() {
    QWidget* page = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    floorMapWidget_ = new FloorMapWidget();
    connect(floorMapWidget_, &FloorMapWidget::tableSelected, this, &MainWindow::onTableSelectedOnMap);
    layout->addWidget(floorMapWidget_, 3);

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
    cardScheduleLabel_->setStyleSheet("background-color: #141620; color: #E5E9F0; padding: 8px; border-radius: 6px; font-family: monospace;");
    sideLayout->addWidget(cardScheduleLabel_);

    sideLayout->addWidget(new QLabel("Отзывы гостей об этом столике:"));
    cardReviewsList_ = new QListWidget();
    sideLayout->addWidget(cardReviewsList_, 1);

    QPushButton* btnBookOnline = new QPushButton("📅 Оформить Онлайн-бронь этого столика");
    btnBookOnline->setStyleSheet("background-color: #2980B9; color: white; font-weight: bold; padding: 9px;");
    connect(btnBookOnline, &QPushButton::clicked, this, &MainWindow::onQuickBookFromMap);
    sideLayout->addWidget(btnBookOnline);

    QPushButton* btnSeatWalkIn = new QPushButton("🚶 Посадить гостей сейчас (В ресторане / Walk-in)");
    btnSeatWalkIn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold; padding: 9px;");
    connect(btnSeatWalkIn, &QPushButton::clicked, this, &MainWindow::onQuickWalkInFromMap);
    sideLayout->addWidget(btnSeatWalkIn);

    layout->addWidget(sideBox, 2);
    return page;
}

QWidget* MainWindow::createBookingPage() {
    QWidget* page = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* formBox = new QGroupBox("Оформление записи (Онлайн или В ресторане • РБ)");
    QVBoxLayout* formLayout = new QVBoxLayout(formBox);

    QFormLayout* fields = new QFormLayout();
    bookingTypeCombo_ = new QComboBox();
    bookingTypeCombo_->addItem("Онлайн-запись заранее (скидка 5% на предзаказ)", "Online");
    bookingTypeCombo_->addItem("В ресторане / Хостес (мгновенная посадка Walk-in)", "WalkIn");
    connect(bookingTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::refreshPreorderWidget);
    fields->addRow("Режим записи:", bookingTypeCombo_);

    guestNameEdit_ = new QLineEdit("Анна Морозова");
    guestNameEdit_->setPlaceholderText("Введите имя и фамилию гостя");
    fields->addRow("Имя гостя:", guestNameEdit_);

    guestPhoneEdit_ = new QLineEdit("+375 (29) 622-33-44");
    guestPhoneEdit_->setPlaceholderText("+375 (29/44/33/25) ___-__-__");
    fields->addRow("Телефон (Беларусь):", guestPhoneEdit_);

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
    bookingCommentEdit_->setPlaceholderText("Например: столик у окна с видом на Свислочь, свечи, без орехов...");
    bookingCommentEdit_->setMaximumHeight(58);
    formLayout->addWidget(bookingCommentEdit_);

    QGroupBox* preBox = new QGroupBox("Предзаказ блюд (цены в BYN • при онлайн-записи скидка 5%)");
    QVBoxLayout* preLayout = new QVBoxLayout(preBox);

    QHBoxLayout* addDishRow = new QHBoxLayout();
    preorderDishCombo_ = new QComboBox();
    addDishRow->addWidget(preorderDishCombo_, 1);

    preorderQtySpin_ = new QSpinBox();
    preorderQtySpin_->setRange(1, 20);
    preorderQtySpin_->setValue(1);
    addDishRow->addWidget(preorderQtySpin_);

    QPushButton* addDishBtn = new QPushButton("+ Добавить");
    connect(addDishBtn, &QPushButton::clicked, this, &MainWindow::onAddDishToPreorder);
    addDishRow->addWidget(addDishBtn);

    QPushButton* remDishBtn = new QPushButton("- Удалить блюдо");
    connect(remDishBtn, &QPushButton::clicked, this, &MainWindow::onRemoveSelectedPreorderItem);
    addDishRow->addWidget(remDishBtn);

    QPushButton* clearPreBtn = new QPushButton("Очистить");
    connect(clearPreBtn, &QPushButton::clicked, this, &MainWindow::onClearPreorder);
    addDishRow->addWidget(clearPreBtn);

    preLayout->addLayout(addDishRow);

    preorderListWidget_ = new QListWidget();
    connect(preorderListWidget_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        onRemoveSelectedPreorderItem();
    });
    preLayout->addWidget(preorderListWidget_);

    preorderTotalLabel_ = new QLabel("Итого по предзаказу: 0.00 BYN");
    preorderTotalLabel_->setStyleSheet("font-weight: bold; color: #F5D76E;");
    preLayout->addWidget(preorderTotalLabel_);

    formLayout->addWidget(preBox);

    QPushButton* submitBtn = new QPushButton("✓ Подтвердить запись / Посадить гостей");
    submitBtn->setStyleSheet("background-color: #D4AF37; color: #181A24; font-weight: bold; font-size: 14px; padding: 10px;");
    connect(submitBtn, &QPushButton::clicked, this, &MainWindow::onCreateReservationClicked);
    formLayout->addWidget(submitBtn);

    QPushButton* waitlistBtn = new QPushButton("⏳ Записать гостя в Лист ожидания (Waitlist)");
    connect(waitlistBtn, &QPushButton::clicked, this, &MainWindow::onAddWaitlistClicked);
    formLayout->addWidget(waitlistBtn);

    layout->addWidget(formBox, 2);

    QGroupBox* listBox = new QGroupBox("Журнал бронирований и посадки на выбранную дату");
    QVBoxLayout* listLayout = new QVBoxLayout(listBox);

    QHBoxLayout* searchRow = new QHBoxLayout();
    searchRow->addWidget(new QLabel("Быстрый поиск брони:"));
    reservationSearchEdit_ = new QLineEdit();
    reservationSearchEdit_->setPlaceholderText("Код BK-..., имя гостя, телефон +375... или комментарий...");
    connect(reservationSearchEdit_, &QLineEdit::textChanged, this, &MainWindow::refreshReservationsAndWaitlist);
    searchRow->addWidget(reservationSearchEdit_, 1);
    listLayout->addLayout(searchRow);

    reservationsTable_ = new QTableWidget(0, 8);
    reservationsTable_->setHorizontalHeaderLabels(
        {"ID", "Код", "Время", "Стол", "Гость", "Персон", "Тип", "Статус"});
    reservationsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    reservationsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    reservationsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listLayout->addWidget(reservationsTable_, 3);

    QHBoxLayout* actionRow1 = new QHBoxLayout();
    QPushButton* btnCheckIn = new QPushButton("Посадить за стол (Check-in)");
    btnCheckIn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold;");
    connect(btnCheckIn, &QPushButton::clicked, this, &MainWindow::onCheckInClicked);
    actionRow1->addWidget(btnCheckIn);

    QPushButton* btnCheckOut = new QPushButton("Закрыть счет (Check-out)");
    btnCheckOut->setStyleSheet("background-color: #2980B9; color: white; font-weight: bold;");
    connect(btnCheckOut, &QPushButton::clicked, this, &MainWindow::onCheckOutClicked);
    actionRow1->addWidget(btnCheckOut);

    QPushButton* btnReschedule = new QPushButton("⇄ Перенести стол / время");
    connect(btnReschedule, &QPushButton::clicked, this, &MainWindow::onRescheduleReservationClicked);
    actionRow1->addWidget(btnReschedule);

    QPushButton* btnCancel = new QPushButton("✕ Отменить бронь");
    btnCancel->setStyleSheet("background-color: #C0392B; color: white; font-weight: bold;");
    connect(btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelReservationClicked);
    actionRow1->addWidget(btnCancel);

    listLayout->addLayout(actionRow1);

    QHBoxLayout* actionRow2 = new QHBoxLayout();
    QPushButton* btnComment = new QPushButton("✎ Изменить комментарий");
    connect(btnComment, &QPushButton::clicked, this, &MainWindow::onEditReservationCommentClicked);
    actionRow2->addWidget(btnComment);

    QPushButton* btnReceipt = new QPushButton("🧾 Показать чек / квитанцию (BYN)");
    connect(btnReceipt, &QPushButton::clicked, this, &MainWindow::onShowReceiptClicked);
    actionRow2->addWidget(btnReceipt);

    QPushButton* btnExportReceipt = new QPushButton("💾 Сохранить квитанцию в TXT");
    connect(btnExportReceipt, &QPushButton::clicked, this, &MainWindow::onExportReceiptClicked);
    actionRow2->addWidget(btnExportReceipt);

    listLayout->addLayout(actionRow2);

    listLayout->addWidget(new QLabel("Лист ожидания (Waitlist) — очередь гостей:"));
    waitlistTable_ = new QTableWidget(0, 6);
    waitlistTable_->setHorizontalHeaderLabels({"ID", "Гость", "Телефон (РБ)", "Дата и час", "Персон", "Комментарий"});
    waitlistTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    waitlistTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    waitlistTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listLayout->addWidget(waitlistTable_, 1);

    QHBoxLayout* wlActions = new QHBoxLayout();
    QPushButton* btnSeatWl = new QPushButton("✓ Посадить выбранного гостя из очереди за свободный стол");
    btnSeatWl->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold;");
    connect(btnSeatWl, &QPushButton::clicked, this, &MainWindow::onSeatFromWaitlistClicked);
    wlActions->addWidget(btnSeatWl);

    QPushButton* btnRemWl = new QPushButton("🗑 Удалить из очереди");
    connect(btnRemWl, &QPushButton::clicked, this, &MainWindow::onRemoveFromWaitlistClicked);
    wlActions->addWidget(btnRemWl);
    listLayout->addLayout(wlActions);

    layout->addWidget(listBox, 3);
    return page;
}

QWidget* MainWindow::createRecommenderPage() {
    QWidget* page = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

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

    recZoneCombo_ = new QComboBox();
    recZoneCombo_->addItems({"Любая зона", "Панорамные окна", "Летняя терраса", "Основной зал", "VIP-каминная", "Барная зона"});
    tForm->addRow("Предпочитаемая зона:", recZoneCombo_);
    tblLayout->addLayout(tForm);

    recWindowCheck_ = new QCheckBox("Хочу столик у панорамного окна (вид на Свислочь)");
    recWindowCheck_->setChecked(true);
    recQuietCheck_ = new QCheckBox("Тихая уединенная зона");
    recQuietCheck_->setChecked(true);
    recOutletCheck_ = new QCheckBox("Нужна розетка 220V / USB");
    recSofaCheck_ = new QCheckBox("Мягкие диваны");

    tblLayout->addWidget(recWindowCheck_);
    tblLayout->addWidget(recQuietCheck_);
    tblLayout->addWidget(recOutletCheck_);
    tblLayout->addWidget(recSofaCheck_);

    QPushButton* runTblRecBtn = new QPushButton("🔍 Найти лучшие свободные столики");
    runTblRecBtn->setStyleSheet("background-color: #D4AF37; color: #181A24; font-weight: bold; padding: 9px;");
    connect(runTblRecBtn, &QPushButton::clicked, this, &MainWindow::onRunTableRecommender);
    tblLayout->addWidget(runTblRecBtn);

    recTablesWidget_ = new QTableWidget(0, 6);
    recTablesWidget_->setHorizontalHeaderLabels({"Столик", "Зона", "Мест", "Окно времени", "Балл", "Почему подходит"});
    recTablesWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    recTablesWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    recTablesWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(recTablesWidget_, &QTableWidget::cellDoubleClicked, this, &MainWindow::onSelectRecommendedTable);
    tblLayout->addWidget(recTablesWidget_, 1);

    tblLayout->addWidget(new QLabel("Совет: дважды кликните по строке рекомендованного стола, чтобы перейти к его бронированию."));

    QGroupBox* menuBox = new QGroupBox("Персональный подбор гастрономического сета (бюджет в BYN)");
    QVBoxLayout* menuLayout = new QVBoxLayout(menuBox);

    QFormLayout* mForm = new QFormLayout();
    recBudgetSpin_ = new QDoubleSpinBox();
    recBudgetSpin_->setRange(20.0, 600.0);
    recBudgetSpin_->setSingleStep(5.0);
    recBudgetSpin_->setValue(95.0);
    recBudgetSpin_->setSuffix(" BYN");
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

    QPushButton* runMenuRecBtn = new QPushButton("🍽 Сформировать персональные сеты блюд (BYN)");
    runMenuRecBtn->setStyleSheet("background-color: #2980B9; color: white; font-weight: bold; padding: 9px;");
    connect(runMenuRecBtn, &QPushButton::clicked, this, &MainWindow::onRunMenuRecommender);
    menuLayout->addWidget(runMenuRecBtn);

    recMenuSetsList_ = new QListWidget();
    menuLayout->addWidget(recMenuSetsList_, 1);

    QPushButton* applySetBtn = new QPushButton("✓ Перенести выбранный сет в корзину предзаказа");
    applySetBtn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold; padding: 9px;");
    connect(applySetBtn, &QPushButton::clicked, this, &MainWindow::onApplyRecommendedMenuSet);
    menuLayout->addWidget(applySetBtn);

    layout->addWidget(tblBox, 1);
    layout->addWidget(menuBox, 1);
    return page;
}

QWidget* MainWindow::createMenuPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* filterBox = new QGroupBox("Фильтрация, поиск и управление меню ресторана (цены в белорусских рублях BYN)");
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
    menuSearchEdit_->setPlaceholderText("Например: драники, палтус, боровики...");
    fLayout->addWidget(menuSearchEdit_, 1);

    fLayout->addWidget(new QLabel("Макс. цена:"));
    menuMaxPriceSpin_ = new QDoubleSpinBox();
    menuMaxPriceSpin_->setRange(0.0, 500.0);
    menuMaxPriceSpin_->setSingleStep(5.0);
    menuMaxPriceSpin_->setValue(0.0);
    menuMaxPriceSpin_->setSuffix(" BYN");
    menuMaxPriceSpin_->setSpecialValueText("Любая");
    fLayout->addWidget(menuMaxPriceSpin_);

    QPushButton* addNewDishBtn = new QPushButton("+ Добавить блюдо");
    addNewDishBtn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold;");
    connect(addNewDishBtn, &QPushButton::clicked, this, &MainWindow::onAddNewDishClicked);
    fLayout->addWidget(addNewDishBtn);

    QPushButton* editDishBtn = new QPushButton("✎ Изменить блюдо");
    connect(editDishBtn, &QPushButton::clicked, this, &MainWindow::onEditDishClicked);
    fLayout->addWidget(editDishBtn);

    QPushButton* delDishBtn = new QPushButton("🗑 Удалить блюдо");
    connect(delDishBtn, &QPushButton::clicked, this, &MainWindow::onDeleteDishClicked);
    fLayout->addWidget(delDishBtn);

    connect(menuCategoryFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterMenuChanged);
    connect(menuTagFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterMenuChanged);
    connect(menuSearchEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterMenuChanged);
    connect(menuMaxPriceSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onFilterMenuChanged);

    layout->addWidget(filterBox);

    menuTableWidget_ = new QTableWidget(0, 8);
    menuTableWidget_->setHorizontalHeaderLabels(
        {"ID", "Блюдо", "Категория", "Цена (BYN)", "Вес и КБЖУ", "Подача", "Рейтинг ★", "Теги и состав"});
    menuTableWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    menuTableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    menuTableWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(menuTableWidget_, 1);

    return page;
}

QWidget* MainWindow::createReviewsAndAnalyticsPage() {
    QWidget* page = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    // Левая часть: Отзывы и комментарии гостей
    QGroupBox* revBox = new QGroupBox("Комментарии, оценки столов/блюд и ответы управляющего");
    QVBoxLayout* revLayout = new QVBoxLayout(revBox);

    QHBoxLayout* revFilterRow = new QHBoxLayout();
    revFilterRow->addWidget(new QLabel("Фильтр по столу:"));
    revFilterTableCombo_ = new QComboBox();
    revFilterTableCombo_->addItem("Все столы", 0);
    for (const auto& t : db_.getAllTables()) {
        revFilterTableCombo_->addItem(QString("Стол #%1 [%2]").arg(t.id).arg(t.code), t.id);
    }
    connect(revFilterTableCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::refreshReviewsAndAnalytics);
    revFilterRow->addWidget(revFilterTableCombo_);

    revFilterRow->addWidget(new QLabel("Фильтр по блюду:"));
    revFilterDishCombo_ = new QComboBox();
    revFilterDishCombo_->addItem("Все блюда", 0);
    connect(revFilterDishCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::refreshReviewsAndAnalytics);
    revFilterRow->addWidget(revFilterDishCombo_, 1);
    revLayout->addLayout(revFilterRow);

    reviewsTable_ = new QTableWidget(0, 6);
    reviewsTable_->setHorizontalHeaderLabels({"ID", "Гость и дата", "Оценка", "Стол / Блюдо", "Комментарий", "Ответ ресторана"});
    reviewsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    reviewsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    reviewsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    revLayout->addWidget(reviewsTable_, 1);

    QGroupBox* addRevBox = new QGroupBox("Оставить новый отзыв или ответить от имени ресторана");
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
    addGrid->addWidget(revDishCombo_, 1, 3);

    addGrid->addWidget(new QLabel("Текст отзыва:"), 2, 0);
    revCommentEdit_ = new QLineEdit();
    revCommentEdit_->setPlaceholderText("Поделитесь впечатлениями о столике, кухне и атмосфере...");
    addGrid->addWidget(revCommentEdit_, 2, 1, 1, 3);

    QPushButton* postRevBtn = new QPushButton("+ Опубликовать отзыв");
    postRevBtn->setStyleSheet("background-color: #27AE60; color: white; font-weight: bold;");
    connect(postRevBtn, &QPushButton::clicked, this, &MainWindow::onAddReviewClicked);
    addGrid->addWidget(postRevBtn, 3, 0, 1, 1);

    QPushButton* replyBtn = new QPushButton("💬 Ответить на выбранный отзыв");
    connect(replyBtn, &QPushButton::clicked, this, &MainWindow::onReplyToReviewClicked);
    addGrid->addWidget(replyBtn, 3, 1, 1, 2);

    QPushButton* delRevBtn = new QPushButton("🗑 Удалить отзыв");
    connect(delRevBtn, &QPushButton::clicked, this, &MainWindow::onDeleteReviewClicked);
    addGrid->addWidget(delRevBtn, 3, 3, 1, 1);

    revLayout->addWidget(addRevBox);

    // Правая часть: Графическая аналитика на QPainter
    QGroupBox* statBox = new QGroupBox("Визуальная аналитика загрузки и выручки (BYN)");
    QVBoxLayout* statLayout = new QVBoxLayout(statBox);

    analyticsSummaryLabel_ = new QLabel();
    analyticsSummaryLabel_->setWordWrap(true);
    analyticsSummaryLabel_->setStyleSheet("font-size: 12px; background-color: #141620; color: #ECEFF4; padding: 10px; border-radius: 8px;");
    statLayout->addWidget(analyticsSummaryLabel_);

    analyticsChartWidget_ = new AnalyticsChartWidget();
    statLayout->addWidget(analyticsChartWidget_, 1);

    QPushButton* exportReportBtn = new QPushButton("💾 Экспортировать отчет за день в TXT-файл");
    exportReportBtn->setStyleSheet("background-color: #2980B9; color: white; font-weight: bold; padding: 9px;");
    connect(exportReportBtn, &QPushButton::clicked, this, &MainWindow::onExportAnalyticsReportClicked);
    statLayout->addWidget(exportReportBtn);

    layout->addWidget(revBox, 3);
    layout->addWidget(statBox, 2);
    return page;
}

void MainWindow::reloadDishCombos() {
    auto dishes = db_.getAllDishes();

    if (preorderDishCombo_) {
        int curId = preorderDishCombo_->currentData().toInt();
        preorderDishCombo_->blockSignals(true);
        preorderDishCombo_->clear();
        for (const auto& d : dishes) {
            preorderDishCombo_->addItem(QString("%1 (%2 BYN)").arg(d.name).arg(d.price, 0, 'f', 2), d.id);
        }
        int idx = preorderDishCombo_->findData(curId);
        if (idx >= 0) preorderDishCombo_->setCurrentIndex(idx);
        preorderDishCombo_->blockSignals(false);
    }

    if (revDishCombo_) {
        int curId = revDishCombo_->currentData().toInt();
        revDishCombo_->blockSignals(true);
        revDishCombo_->clear();
        revDishCombo_->addItem("Не привязано", 0);
        for (const auto& d : dishes) {
            revDishCombo_->addItem(d.name, d.id);
        }
        int idx = revDishCombo_->findData(curId);
        if (idx >= 0) revDishCombo_->setCurrentIndex(idx);
        revDishCombo_->blockSignals(false);
    }

    if (revFilterDishCombo_) {
        int curId = revFilterDishCombo_->currentData().toInt();
        revFilterDishCombo_->blockSignals(true);
        revFilterDishCombo_->clear();
        revFilterDishCombo_->addItem("Все блюда", 0);
        for (const auto& d : dishes) {
            revFilterDishCombo_->addItem(d.name, d.id);
        }
        int idx = revFilterDishCombo_->findData(curId);
        if (idx >= 0) revFilterDishCombo_->setCurrentIndex(idx);
        revFilterDishCombo_->blockSignals(false);
    }
}

void MainWindow::onDateTimeFilterChanged() {
    if (timeSlider_ && timeSlider_->value() != hourSpin_->value()) {
        timeSlider_->blockSignals(true);
        timeSlider_->setValue(hourSpin_->value());
        timeSlider_->blockSignals(false);
    }

    int maxAllowedDuration = std::max(1, std::min(4, DatabaseManager::CLOSE_HOUR - hourSpin_->value()));
    durationSpin_->blockSignals(true);
    durationSpin_->setMaximum(maxAllowedDuration);
    if (durationSpin_->value() > maxAllowedDuration) {
        durationSpin_->setValue(maxAllowedDuration);
    }
    durationSpin_->blockSignals(false);

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
    QMap<int, int> freeHrsMap;
    QMap<int, QString> guestBadges = db_.getActiveGuestBadgeByTable(date, hour, dur);
    int freeCount = 0;
    int seatedTablesCount = 0;

    for (const auto& t : tables) {
        int st = db_.getTableOccupancyStatus(t.id, date, hour, dur);
        occMap[t.id] = st;
        ratMap[t.id] = db_.getAverageTableRating(t.id);
        freeHrsMap[t.id] = db_.getFreeHoursFrom(t.id, date, hour);
        if (st == 0) ++freeCount;
        if (st == 2) ++seatedTablesCount;
    }

    QMap<int, int> selHourly;
    for (int h = 10; h <= 22; ++h) {
        selHourly[h] = db_.getTableOccupancyStatus(currentTableId_, date, h, 1);
    }

    floorMapWidget_->updateFloorData(tables, occMap, ratMap, guestBadges, freeHrsMap, selHourly);

    // Обновление 4 верхних KPI-карточек
    double dayRevenue = 0.0;
    int dayGuests = 0;
    for (const auto& b : db_.getReservationsByDate(date)) {
        if (b.status == "Cancelled") continue;
        dayGuests += b.partySize;
        double pre = db_.calculatePreorderTotal(b.preorder, b.bookingType == "Online");
        TableInfo ti = db_.getTableById(b.tableId);
        dayRevenue += std::max(pre, ti.minDeposit);
    }

    kpiFreeTablesLabel_->setText(QString("%1 из %2 столов (%3:00)").arg(freeCount).arg(tables.size()).arg(hour, 2, 10, QChar('0')));
    kpiSeatedGuestsLabel_->setText(QString("Занято столов: %1 | Гостей за день: %2").arg(seatedTablesCount).arg(dayGuests));
    kpiWaitlistLabel_->setText(QString("В очереди: %1 групп").arg(db_.getWaitlist().size()));
    kpiRevenueRatingLabel_->setText(QString("%1 BYN  •  ★ %2 / 5.0").arg(dayRevenue, 0, 'f', 2).arg(db_.getOverallRating(), 0, 'f', 2));

    onTableSelectedOnMap(currentTableId_);
}

void MainWindow::onTableSelectedOnMap(int tableId) {
    currentTableId_ = tableId;
    TableInfo t = db_.getTableById(tableId);
    if (t.id == 0) return;

    QString date = activeDateString();
    QMap<int, int> selHourly;
    for (int h = 10; h <= 22; ++h) {
        selHourly[h] = db_.getTableOccupancyStatus(currentTableId_, date, h, 1);
    }
    floorMapWidget_->update();

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
        ? QString("<b>Минимальный депозит:</b> %1 BYN").arg(t.minDeposit, 0, 'f', 2)
        : "<b>Депозит:</b> Без обязательного депозита");

    double avgR = db_.getAverageTableRating(t.id);
    cardRatingLabel_->setText(avgR > 0.0
        ? QString("Рейтинг стола по отзывам: ★ %1 / 5.0").arg(avgR, 0, 'f', 1)
        : "Рейтинг стола: пока нет оценок");

    QString sched = QString("Почасовой график на %1:\n").arg(date);
    for (int h = 10; h < 23; ++h) {
        int st = db_.getTableOccupancyStatus(t.id, date, h, 1);
        QString tag = (st == 0) ? "[Своб]" : (st == 1 ? "[Бронь]" : "[Занят]");
        sched += QString("%1:00%2 ").arg(h, 2, 10, QChar('0')).arg(tag);
        if (h == 15) sched += "\n";
    }
    cardScheduleLabel_->setText(sched);

    cardReviewsList_->clear();
    for (const auto& rev : db_.getAllReviews(t.id, 0)) {
        cardReviewsList_->addItem(QString("★ %1/5 — %2 (%3): «%4»")
                                  .arg(rev.rating).arg(rev.authorName).arg(rev.reviewDate).arg(rev.comment));
    }
    if (cardReviewsList_->count() == 0) {
        cardReviewsList_->addItem("Пока нет отзывов об этом столике.");
    }
}

void MainWindow::onQuickBookFromMap() {
    int idx = tableSelectCombo_->findData(currentTableId_);
    if (idx >= 0) tableSelectCombo_->setCurrentIndex(idx);
    bookingTypeCombo_->setCurrentIndex(0);
    switchToPage(1);
}

void MainWindow::onQuickWalkInFromMap() {
    int idx = tableSelectCombo_->findData(currentTableId_);
    if (idx >= 0) tableSelectCombo_->setCurrentIndex(idx);
    bookingTypeCombo_->setCurrentIndex(1);
    switchToPage(1);
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

void MainWindow::onRemoveSelectedPreorderItem() {
    int row = preorderListWidget_->currentRow();
    if (row < 0 || row >= static_cast<int>(currentPreorder_.size())) return;
    currentPreorder_.erase(currentPreorder_.begin() + row);
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
            QString("%1 x%2 = %3 BYN (двойной клик — удалить)")
            .arg(item.dishName).arg(item.quantity).arg(item.unitPrice * item.quantity, 0, 'f', 2));
    }
    bool isOnline = (bookingTypeCombo_->currentData().toString() == "Online");
    double total = db_.calculatePreorderTotal(currentPreorder_, isOnline);
    if (isOnline && !currentPreorder_.empty()) {
        preorderTotalLabel_->setText(QString("Итого со скидкой 5% (онлайн-предзаказ): %1 BYN").arg(total, 0, 'f', 2));
    } else {
        preorderTotalLabel_->setText(QString("Итого по предзаказу: %1 BYN").arg(total, 0, 'f', 2));
    }
}

void MainWindow::onCreateReservationClicked() {
    if (guestNameEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Проверка данных", "Пожалуйста, укажите имя гостя.");
        guestNameEdit_->setFocus();
        return;
    }

    ReservationInfo res;
    res.guestName = guestNameEdit_->text().trimmed();
    res.guestPhone = guestPhoneEdit_->text().trimmed();
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
    QString errMsg;
    int newId = db_.createReservation(res, code, &errMsg);
    if (newId == 0) {
        QMessageBox::warning(this, "Невозможно оформить бронь",
            errMsg + "\n\nВоспользуйтесь разделом «Рекомендации» или добавьте гостя в Лист ожидания.");
        return;
    }

    currentPreorder_.clear();
    bookingCommentEdit_->clear();
    refreshAllViews();

    QMessageBox::information(this, "Запись успешно создана",
        QString("Бронирование %1 успешно сохранено в базе SQLite!\nСтолик #%2 на %3 (%4:00–%5:00).")
        .arg(code).arg(res.tableId).arg(res.visitDate).arg(res.startHour).arg(res.startHour + res.durationHours));
}

int MainWindow::selectedReservationIdFromTable() const {
    auto items = reservationsTable_->selectedItems();
    if (items.isEmpty()) return 0;
    int row = items.first()->row();
    return reservationsTable_->item(row, 0)->text().toInt();
}

int MainWindow::selectedWaitlistIdFromTable() const {
    auto items = waitlistTable_->selectedItems();
    if (items.isEmpty()) return 0;
    int row = items.first()->row();
    return waitlistTable_->item(row, 0)->text().toInt();
}

int MainWindow::selectedDishIdFromTable() const {
    auto items = menuTableWidget_->selectedItems();
    if (items.isEmpty()) return 0;
    int row = items.first()->row();
    return menuTableWidget_->item(row, 0)->text().toInt();
}

void MainWindow::onCheckInClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) {
        QMessageBox::information(this, "Выбор брони", "Выберите бронь в таблице выше.");
        return;
    }
    QString err;
    if (!db_.updateReservationStatus(id, "Seated", &err)) {
        QMessageBox::warning(this, "Ошибка смены статуса", err);
        return;
    }
    refreshAllViews();
}

void MainWindow::onCheckOutClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) {
        QMessageBox::information(this, "Выбор брони", "Выберите бронь в таблице выше.");
        return;
    }
    QString err;
    if (!db_.updateReservationStatus(id, "Completed", &err)) {
        QMessageBox::warning(this, "Ошибка смены статуса", err);
        return;
    }
    refreshAllViews();
}

void MainWindow::onCancelReservationClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) {
        QMessageBox::information(this, "Выбор брони", "Выберите бронь в таблице выше.");
        return;
    }
    QString err;
    if (db_.updateReservationStatus(id, "Cancelled", &err)) {
        refreshAllViews();
    }
}

void MainWindow::onRescheduleReservationClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) {
        QMessageBox::information(this, "Выбор брони", "Выберите бронь в таблице для переноса стола или времени.");
        return;
    }
    ReservationInfo r = db_.getReservationById(id);
    if (r.id == 0) return;

    bool ok = false;
    int newTableId = QInputDialog::getInt(this, "Перенос брони " + r.bookingCode,
                                          "Новый номер столика (1..12):", r.tableId, 1, 12, 1, &ok);
    if (!ok) return;

    int newHour = QInputDialog::getInt(this, "Перенос брони " + r.bookingCode,
                                       "Новый час начала (10..22):", r.startHour,
                                       DatabaseManager::OPEN_HOUR, DatabaseManager::CLOSE_HOUR - 1, 1, &ok);
    if (!ok) return;

    int maxDur = std::min(4, DatabaseManager::CLOSE_HOUR - newHour);
    int newDur = QInputDialog::getInt(this, "Перенос брони " + r.bookingCode,
                                      "Длительность (часов):", std::min(r.durationHours, maxDur), 1, maxDur, 1, &ok);
    if (!ok) return;

    QString err;
    if (!db_.rescheduleReservation(id, newTableId, r.visitDate, newHour, newDur, &err)) {
        QMessageBox::warning(this, "Не удалось перенести бронь", err);
        return;
    }
    refreshAllViews();
    QMessageBox::information(this, "Бронь перенесена",
                             QString("Бронь %1 успешно перенесена на стол #%2 (%3:00–%4:00).")
                             .arg(r.bookingCode).arg(newTableId).arg(newHour).arg(newHour + newDur));
}

void MainWindow::onEditReservationCommentClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) {
        QMessageBox::information(this, "Выбор брони", "Выберите бронь в таблице выше.");
        return;
    }
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
    if (id == 0) {
        QMessageBox::information(this, "Выбор брони", "Выберите бронь в таблице для просмотра квитанции.");
        return;
    }
    QMessageBox::information(this, "Квитанция бронирования (BYN)", db_.formatReservationReceipt(id));
}

void MainWindow::onExportReceiptClicked() {
    int id = selectedReservationIdFromTable();
    if (id == 0) {
        QMessageBox::information(this, "Выбор брони", "Выберите бронь в таблице для сохранения квитанции.");
        return;
    }
    ReservationInfo r = db_.getReservationById(id);
    QString path = QFileDialog::getSaveFileName(this, "Сохранить квитанцию",
                                                QString("receipt_%1.txt").arg(r.bookingCode),
                                                "Text Files (*.txt)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#endif
        out << db_.formatReservationReceipt(id);
        file.close();
        QMessageBox::information(this, "Экспорт выполнен", "Квитанция успешно сохранена в файл:\n" + path);
    }
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
        refreshAllViews();
        QMessageBox::information(this, "Лист ожидания", "Гость успешно добавлен в Лист ожидания (Waitlist).");
    }
}

void MainWindow::onSeatFromWaitlistClicked() {
    int wlId = selectedWaitlistIdFromTable();
    if (wlId == 0) {
        QMessageBox::information(this, "Лист ожидания", "Выберите гостя в таблице Листа ожидания внизу.");
        return;
    }
    QString code;
    QString err;
    int resId = db_.seatFromWaitlist(wlId, currentTableId_, code, &err);
    if (resId == 0) {
        QMessageBox::warning(this, "Нет свободного стола", err);
        return;
    }
    refreshAllViews();
    ReservationInfo r = db_.getReservationById(resId);
    QMessageBox::information(this, "Гость посажен за стол",
        QString("Гость «%1» из листа ожидания успешно размещен за столиком #%2 (код записи: %3)!")
        .arg(r.guestName).arg(r.tableId).arg(code));
}

void MainWindow::onRemoveFromWaitlistClicked() {
    int wlId = selectedWaitlistIdFromTable();
    if (wlId == 0) return;
    if (db_.removeFromWaitlist(wlId)) {
        refreshAllViews();
    }
}

void MainWindow::refreshReservationsAndWaitlist() {
    QString kw = reservationSearchEdit_ ? reservationSearchEdit_->text() : "";
    auto list = db_.getReservationsByDate(activeDateString(), kw);
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
        waitlistTable_->setItem(row, 4, new QTableWidgetItem(QString::number(wl[i].partySize)));
        waitlistTable_->setItem(row, 5, new QTableWidgetItem(wl[i].comment));
    }
}

void MainWindow::onRunTableRecommender() {
    TablePreference pref;
    pref.partySize = recPartySpin_->value();
    pref.visitDate = activeDateString();
    pref.startHour = hourSpin_->value();
    pref.durationHours = durationSpin_->value();
    pref.occasion = recOccasionCombo_->currentText();
    pref.preferredZone = recZoneCombo_->currentText();
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
        recTablesWidget_->setItem(row, 3, new QTableWidgetItem(QString("Свободен %1 ч.").arg(r.freeHoursWindow)));
        recTablesWidget_->setItem(row, 4, new QTableWidgetItem(QString("%1 / 100").arg(r.matchScore)));
        recTablesWidget_->setItem(row, 5, new QTableWidgetItem(r.reasons.join("; ")));
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
    switchToPage(1);
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
        QString text = QString("%1 — %2 BYN (%3 ккал | подача ~%4 мин)\n   %5\n")
                       .arg(s.title).arg(s.totalPrice, 0, 'f', 2).arg(s.totalCalories).arg(s.maxPrepTimeMin).arg(s.explanation);
        for (const auto& d : s.dishes) {
            text += QString("   • [%1] %2 — %3 BYN (%4 ккал)\n")
                    .arg(d.category).arg(d.name).arg(d.price, 0, 'f', 2).arg(d.calories);
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
    switchToPage(1);
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
        menuTableWidget_->setItem(row, 3, new QTableWidgetItem(QString("%1 BYN").arg(d.price, 0, 'f', 2)));
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
                                        {"Закуски", "Супы", "Горячее", "Десерты", "Напитки"}, 2, false, &ok);
    if (!ok) return;

    double price = QInputDialog::getDouble(this, "Цена (BYN)", "Цена в белорусских рублях (BYN):", 32.0, 2.0, 1000.0, 2, &ok);
    if (!ok) return;

    int calories = QInputDialog::getInt(this, "Калорийность", "Калории (ккал):", 380, 10, 3000, 10, &ok);
    if (!ok) return;

    QString desc = QInputDialog::getText(this, "Описание блюда", "Состав и подача:",
                                         QLineEdit::Normal, "Авторское блюдо от шеф-повара", &ok);
    if (!ok) return;

    DishInfo d;
    d.name = name.trimmed();
    d.category = cat;
    d.price = price;
    d.weightGrams = 250;
    d.calories = calories;
    d.prepTimeMin = 15;
    d.tags = {"хит шефа"};
    d.description = desc.trimmed();

    if (db_.addDish(d)) {
        reloadDishCombos();
        refreshMenuTable();
        onRunMenuRecommender();
    }
}

void MainWindow::onEditDishClicked() {
    int dishId = selectedDishIdFromTable();
    if (dishId == 0) {
        QMessageBox::information(this, "Выбор блюда", "Выберите блюдо в таблице меню для редактирования.");
        return;
    }
    DishInfo d = db_.getDishById(dishId);
    if (d.id == 0) return;

    bool ok = false;
    double newPrice = QInputDialog::getDouble(this, "Изменить цену (BYN)",
                                              QString("Новая цена для «%1» (BYN):").arg(d.name),
                                              d.price, 2.0, 1000.0, 2, &ok);
    if (!ok) return;

    QString newDesc = QInputDialog::getText(this, "Изменить описание", "Состав и описание:",
                                            QLineEdit::Normal, d.description, &ok);
    if (!ok) return;

    d.price = newPrice;
    d.description = newDesc.trimmed();
    if (db_.updateDish(d)) {
        reloadDishCombos();
        refreshAllViews();
    }
}

void MainWindow::onDeleteDishClicked() {
    int dishId = selectedDishIdFromTable();
    if (dishId == 0) {
        QMessageBox::information(this, "Выбор блюда", "Выберите блюдо в таблице меню для удаления.");
        return;
    }
    DishInfo d = db_.getDishById(dishId);
    if (QMessageBox::question(this, "Удаление блюда",
                              QString("Удалить блюдо «%1» из базы данных SQLite?").arg(d.name)) == QMessageBox::Yes) {
        if (db_.deleteDish(dishId)) {
            reloadDishCombos();
            refreshAllViews();
        }
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
    if (items.isEmpty()) {
        QMessageBox::information(this, "Выбор отзыва", "Выберите отзыв в таблице для ответа.");
        return;
    }
    int revId = reviewsTable_->item(items.first()->row(), 0)->text().toInt();
    bool ok = false;
    QString reply = QInputDialog::getText(this, "Ответ ресторана", "Введите ответ управляющего на отзыв:",
                                          QLineEdit::Normal, "Благодарим за ваш отзыв! Ждем вас снова в Минске.", &ok);
    if (ok && !reply.trimmed().isEmpty()) {
        db_.addManagerReply(revId, reply.trimmed());
        refreshReviewsAndAnalytics();
    }
}

void MainWindow::onDeleteReviewClicked() {
    auto items = reviewsTable_->selectedItems();
    if (items.isEmpty()) return;
    int revId = reviewsTable_->item(items.first()->row(), 0)->text().toInt();
    if (db_.deleteReview(revId)) {
        refreshAllViews();
    }
}

void MainWindow::onExportAnalyticsReportClicked() {
    QString date = activeDateString();
    QString path = QFileDialog::getSaveFileName(this, "Сохранить аналитический отчет",
                                                QString("analytics_report_%1.txt").arg(date),
                                                "Text Files (*.txt)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#endif
        out << db_.formatDailyAnalyticsReport(date);
        file.close();
        QMessageBox::information(this, "Отчет сохранен", "Аналитический отчет успешно сохранен:\n" + path);
    }
}

void MainWindow::refreshReviewsAndAnalytics() {
    int filterTbl = revFilterTableCombo_ ? revFilterTableCombo_->currentData().toInt() : 0;
    int filterDish = revFilterDishCombo_ ? revFilterDishCombo_->currentData().toInt() : 0;

    auto revs = db_.getAllReviews(filterTbl, filterDish);
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

    auto allRevs = db_.getAllReviews();
    analyticsSummaryLabel_->setText(
        QString("<b>Сводка за %1 (Республика Беларусь • BYN):</b><br>"
                "• Активных записей: <b>%2</b> (Онлайн: %3 | В ресторане Walk-in: %4)<br>"
                "• Ожидаемое число гостей: <b>%5 чел.</b><br>"
                "• Выручка по предзаказам и депозитам: <b>%6 BYN</b><br>"
                "• Средний рейтинг ресторана: <b>★ %7 / 5.0</b> (всего отзывов: %8)")
        .arg(date)
        .arg(onlineCnt + walkInCnt).arg(onlineCnt).arg(walkInCnt)
        .arg(totalGuests)
        .arg(revenue, 0, 'f', 2)
        .arg(db_.getOverallRating(), 0, 'f', 2)
        .arg(allRevs.size()));

    if (analyticsChartWidget_) {
        analyticsChartWidget_->updateCharts(
            db_.getHourlyOccupiedTablesCount(date),
            static_cast<int>(db_.getAllTables().size()),
            db_.getRevenueByZone(date));
    }
}

} // namespace Restaurant
