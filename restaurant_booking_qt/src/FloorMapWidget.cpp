#include "FloorMapWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QToolTip>
#include <QtGlobal>
#include <cmath>

namespace Restaurant {

FloorMapWidget::FloorMapWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(720, 540);
    setMouseTracking(true);

    pulseTimer_ = new QTimer(this);
    connect(pulseTimer_, &QTimer::timeout, this, [this]() {
        pulsePhase_ += 0.18;
        if (pulsePhase_ > 6.283185) pulsePhase_ -= 6.283185;
        update();
    });
    pulseTimer_->start(50);
}

void FloorMapWidget::updateFloorData(const std::vector<TableInfo>& tables,
                                     const QMap<int, int>& occupancyByTableId,
                                     const QMap<int, double>& ratingByTableId,
                                     const QMap<int, QString>& guestBadgeByTableId,
                                     const QMap<int, int>& freeHoursByTableId,
                                     const QMap<int, int>& selectedTableHourlyStatus) {
    tables_ = tables;
    occupancy_ = occupancyByTableId;
    ratings_ = ratingByTableId;
    guestBadges_ = guestBadgeByTableId;
    freeHours_ = freeHoursByTableId;
    selectedHourly_ = selectedTableHourlyStatus;
    update();
}

void FloorMapWidget::setSelectedTableId(int tableId) {
    if (selectedTableId_ != tableId) {
        selectedTableId_ = tableId;
        update();
        emit tableSelected(selectedTableId_);
    }
}

QRectF FloorMapWidget::computeTableRect(const TableInfo& t) const {
    const double topMargin = 82.0;
    const double bottomMargin = 86.0;
    const double leftMargin = 28.0;
    const double rightMargin = 28.0;

    double availW = std::max(420.0, width() - leftMargin - rightMargin);
    double availH = std::max(300.0, height() - topMargin - bottomMargin);

    double cellW = availW / 4.0;
    double cellH = availH / 3.0;

    double padX = 16.0;
    double padY = 14.0;

    double x = leftMargin + t.gridX * cellW + padX;
    double y = topMargin + t.gridY * cellH + padY;
    double w = cellW - 2.0 * padX;
    double h = cellH - 2.0 * padY;
    return QRectF(x, y, w, h);
}

void FloorMapWidget::drawArchitecturalDecor(QPainter& p) const {
    // 1. Паркетная/плиточная текстура пола
    p.setPen(QPen(QColor(34, 38, 52), 1.0));
    for (int x = 28; x < width() - 28; x += 44) {
        p.drawLine(x, 76, x, height() - 84);
    }
    for (int y = 76; y < height() - 84; y += 44) {
        p.drawLine(28, y, width() - 28, y);
    }

    // 2. Свечение живого камина в левом нижнем углу (VIP-зона)
    QPointF fireplaceCenter(24.0, height() - 140.0);
    QRadialGradient fireGlow(fireplaceCenter, 95.0);
    fireGlow.setColorAt(0.0, QColor(255, 120, 30, 110));
    fireGlow.setColorAt(0.5, QColor(212, 175, 55, 45));
    fireGlow.setColorAt(1.0, QColor(22, 24, 34, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(fireGlow);
    p.drawEllipse(fireplaceCenter, 95.0, 95.0);

    // 3. Декоративные зеленые растения в кадках между зонами
    p.setBrush(QColor(39, 174, 96, 170));
    p.setPen(QPen(QColor(24, 106, 59), 1.5));
    const double midX = width() / 2.0;
    p.drawEllipse(QPointF(midX, 82.0), 8.0, 8.0);
    p.drawEllipse(QPointF(midX, height() - 92.0), 8.0, 8.0);
}

void FloorMapWidget::drawChairsAndSofas(QPainter& p, const TableInfo& t, const QRectF& r) const {
    p.setPen(QPen(QColor(40, 44, 60), 1.0));
    if (t.hasSofa) {
        // Длинные мягкие диваны сверху и снизу стола
        p.setBrush(QColor(135, 112, 170, 200));
        p.drawRoundedRect(QRectF(r.left() + 8, r.top() - 7, r.width() - 16, 6), 3, 3);
        p.drawRoundedRect(QRectF(r.left() + 8, r.bottom() + 1, r.width() - 16, 6), 3, 3);
        if (t.capacity >= 6) {
            p.drawRoundedRect(QRectF(r.left() - 6, r.top() + 12, 5, r.height() - 24), 2, 2);
            p.drawRoundedRect(QRectF(r.right() + 1, r.top() + 12, 5, r.height() - 24), 2, 2);
        }
    } else {
        // Индивидуальные стулья по числу мест
        p.setBrush(QColor(110, 125, 148, 200));
        int pairs = std::max(1, t.capacity / 2);
        double step = (r.width() - 24.0) / std::max(1, pairs);
        for (int i = 0; i < pairs; ++i) {
            double cx = r.left() + 12.0 + step * (i + 0.5);
            p.drawEllipse(QPointF(cx, r.top() - 4.5), 6.5, 3.5);
            p.drawEllipse(QPointF(cx, r.bottom() + 4.5), 6.5, 3.5);
        }
    }
}

void FloorMapWidget::drawBottomGanttStrip(QPainter& p) const {
    QRectF stripBox(22.0, height() - 42.0, width() - 44.0, 34.0);
    p.setPen(QPen(QColor(65, 72, 96), 1.2));
    p.setBrush(QColor(26, 29, 42));
    p.drawRoundedRect(stripBox, 6, 6);

    QFont labelF = font();
    labelF.setBold(true);
    labelF.setPointSize(8);
    p.setFont(labelF);
    p.setPen(QColor(245, 215, 110));

    QRectF titleRect(stripBox.left() + 10, stripBox.top(), 175, stripBox.height());
    p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
               QString("Почасовая шкала стола #%1:").arg(selectedTableId_));

    double barStart = stripBox.left() + 190.0;
    double barAvailW = stripBox.width() - 200.0;
    int hoursCount = 13; // 10:00 .. 22:00
    double slotW = barAvailW / hoursCount;

    for (int h = 10; h <= 22; ++h) {
        int idx = h - 10;
        QRectF slotRect(barStart + idx * slotW + 2.0, stripBox.top() + 5.0, slotW - 4.0, stripBox.height() - 10.0);
        int st = selectedHourly_.value(h, 0);
        QColor c = (st == 0) ? QColor(39, 174, 96) : (st == 1 ? QColor(243, 156, 18) : QColor(231, 76, 60));
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(slotRect, 3, 3);

        p.setPen(QColor(18, 20, 26));
        p.drawText(slotRect, Qt::AlignCenter, QString("%1:00").arg(h, 2, 10, QChar('0')));
    }
}

void FloorMapWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 1. Фон зала
    p.fillRect(rect(), QColor(20, 22, 32));
    drawArchitecturalDecor(p);

    // 2. Легенда статусов сверху
    QRectF legendRect(22, 6, width() - 44, 22);
    QFont legendFont = font();
    legendFont.setPointSize(8);
    legendFont.setBold(true);
    p.setFont(legendFont);
    p.setPen(QColor(215, 220, 230));
    p.drawText(legendRect, Qt::AlignCenter,
               "🟢 СВОБОДЕН (доступен)     🟡 БРОНЬ (онлайн-запись)     "
               "🔴 ЗАНЯТ (гости в зале)     ✨ Пульсирующая золотая рамка = выбранный стол");

    // 3. Верхний архитектурный ориентир — Панорамные окна на Свислочь и Терраса
    QRectF northBanner(22, 32, width() - 44, 28);
    QLinearGradient winGrad(northBanner.topLeft(), northBanner.bottomRight());
    winGrad.setColorAt(0.0, QColor(28, 52, 84));
    winGrad.setColorAt(1.0, QColor(24, 68, 64));
    p.setPen(QPen(QColor(82, 152, 210), 1.5));
    p.setBrush(winGrad);
    p.drawRoundedRect(northBanner, 6, 6);
    p.setPen(QColor(195, 230, 255));
    QFont bannerFont = font();
    bannerFont.setBold(true);
    bannerFont.setPointSize(9);
    p.setFont(bannerFont);
    p.drawText(northBanner, Qt::AlignCenter,
               "ПАНОРАМНЫЕ ОКНА С ВИДОМ НА СВИСЛОЧЬ И СТАРЫЙ МИНСК (W1–W2)   |   ЛЕТНЯЯ ТЕРРАСА (T1–T2)");

    // 4. Ориентир над нижней шкалой — Камин, Хостес, Бар
    QRectF southBanner(22, height() - 72, width() - 44, 24);
    p.setPen(QPen(QColor(125, 102, 68), 1.2));
    p.setBrush(QColor(44, 36, 32));
    p.drawRoundedRect(southBanner, 5, 5);
    p.setPen(QColor(235, 205, 150));
    p.drawText(southBanner, Qt::AlignCenter,
               "🔥 КАМИННЫЙ VIP-ЗАЛ (V1–V2)   |   ГЛАВНЫЙ ВХОД И ХОСТЕС   |   🍸 КОНТАКТНЫЙ БАР И КУХНЯ (B1–B2)");

    // 5. Отрисовка каждого столика
    tableRects_.clear();
    for (const auto& t : tables_) {
        QRectF r = computeTableRect(t);
        tableRects_[t.id] = r;

        int st = occupancy_.value(t.id, 0);
        QColor fillCol;
        QColor borderCol;
        QString statusText;

        if (st == 0) {
            fillCol = QColor(28, 70, 50);
            borderCol = QColor(46, 204, 113);
            int fHrs = freeHours_.value(t.id, 0);
            statusText = (fHrs >= 4) ? "СВОБОДЕН" : QString("СВОБ (%1 ч)").arg(fHrs);
        } else if (st == 1) {
            fillCol = QColor(88, 64, 22);
            borderCol = QColor(243, 156, 18);
            statusText = guestBadges_.value(t.id, "БРОНЬ");
        } else {
            fillCol = QColor(88, 30, 36);
            borderCol = QColor(231, 76, 60);
            statusText = guestBadges_.value(t.id, "ЗАНЯТ");
        }

        bool isSelected = (t.id == selectedTableId_);
        bool isHovered = (t.id == hoveredTableId_);

        if (isHovered) {
            fillCol = fillCol.lighter(120);
        }

        drawChairsAndSofas(p, t, r);

        // Пульсирующее свечение вокруг выбранного стола
        if (isSelected) {
            int alpha = static_cast<int>(120 + 85 * std::sin(pulsePhase_));
            QPen glowPen(QColor(255, 215, 0, alpha), 6.0);
            p.setPen(glowPen);
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r.adjusted(-3, -3, 3, 3), 14, 14);
        }

        // Форма стола (более скругленная для камерных столов на 2 чел.)
        double cornerRadius = (t.capacity <= 2) ? 18.0 : 10.0;
        QPen tablePen(isSelected ? QColor(255, 225, 70) : borderCol, isSelected ? 2.8 : 1.8);
        p.setPen(tablePen);
        p.setBrush(fillCol);
        p.drawRoundedRect(r, cornerRadius, cornerRadius);

        double hStep = r.height() / 4.2;

        // Строка 1: Номер и код стола + вместимость
        QFont titleF = font();
        titleF.setBold(true);
        titleF.setPointSize(9);
        p.setFont(titleF);
        p.setPen(Qt::white);

        QRectF topRow(r.left() + 8, r.top() + 3, r.width() - 16, hStep);
        p.drawText(topRow, Qt::AlignLeft | Qt::AlignVCenter,
                   QString("#%1 [%2]").arg(t.id).arg(t.code));
        p.drawText(topRow, Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1 чел.").arg(t.capacity));

        // Строка 2: Зона и рейтинг
        QFont subF = font();
        subF.setPointSize(8);
        subF.setBold(false);
        p.setFont(subF);
        p.setPen(QColor(215, 220, 230));

        QRectF zoneRow(r.left() + 8, r.top() + 3 + hStep, r.width() - 16, hStep);
        p.drawText(zoneRow, Qt::AlignLeft | Qt::AlignVCenter, t.zone);

        double rt = ratings_.value(t.id, 0.0);
        if (rt > 0.0) {
            p.setPen(QColor(255, 215, 90));
            p.drawText(zoneRow, Qt::AlignRight | Qt::AlignVCenter,
                       QString("★ %1").arg(rt, 0, 'f', 1));
        }

        // Строка 3: Значки оснащения и депозита в BYN
        QStringList feats;
        if (t.nearWindow) feats << "Окно";
        if (t.hasSofa)    feats << "Диван";
        if (t.hasOutlet)  feats << "220V";
        if (t.isQuiet)    feats << "Тихо";
        if (t.minDeposit > 0.0) feats << QString("%1 BYN").arg(t.minDeposit, 0, 'f', 0);
        if (feats.isEmpty()) feats << "Стандарт";

        p.setPen(QColor(175, 198, 220));
        QRectF featRow(r.left() + 8, r.top() + 3 + hStep * 2.0, r.width() - 16, hStep);
        p.drawText(featRow, Qt::AlignLeft | Qt::AlignVCenter, feats.join(" • "));

        // Строка 4: Плашка статуса внизу карточки стола
        double badgeH = std::min(19.0, std::max(15.0, hStep * 0.9));
        QRectF badgeRect(r.left() + 8, r.bottom() - badgeH - 4, r.width() - 16, badgeH);
        p.setPen(Qt::NoPen);
        p.setBrush(borderCol);
        p.drawRoundedRect(badgeRect, 4, 4);

        QFont badgeFont = font();
        badgeFont.setBold(true);
        badgeFont.setPointSize(8);
        p.setFont(badgeFont);
        p.setPen(QColor(18, 20, 26));
        p.drawText(badgeRect, Qt::AlignCenter, statusText);
    }

    // 6. Нижняя визуальная шкала Gantt для выбранного стола
    drawBottomGanttStrip(p);
}

void FloorMapWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        for (auto it = tableRects_.constBegin(); it != tableRects_.constEnd(); ++it) {
            if (it.value().contains(event->pos())) {
                selectedTableId_ = it.key();
                update();
                emit tableSelected(selectedTableId_);
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void FloorMapWidget::mouseMoveEvent(QMouseEvent* event) {
    int found = 0;
    for (auto it = tableRects_.constBegin(); it != tableRects_.constEnd(); ++it) {
        if (it.value().contains(event->pos())) {
            found = it.key();
            break;
        }
    }
    if (found != hoveredTableId_) {
        hoveredTableId_ = found;
        update();
    }
    if (found > 0) {
        for (const auto& t : tables_) {
            if (t.id == found) {
                int fHrs = freeHours_.value(t.id, 0);
                QString depStr = (t.minDeposit > 0.0)
                    ? QString("%1 BYN").arg(t.minDeposit, 0, 'f', 2)
                    : "без депозита";
                QString tip = QString("<b>Столик #%1 [%2]</b> — %3<br>"
                                      "Вместимость: до %4 чел. | Депозит: %5<br>"
                                      "Свободное окно от текущего часа: %6 ч.<br>%7")
                              .arg(t.id).arg(t.code).arg(t.zone).arg(t.capacity).arg(depStr).arg(fHrs).arg(t.description);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                QPoint globalPt = event->globalPosition().toPoint();
#else
                QPoint globalPt = event->globalPos();
#endif
                QToolTip::showText(globalPt, tip, this);
                break;
            }
        }
    }
    QWidget::mouseMoveEvent(event);
}

} // namespace Restaurant
