#include "FloorMapWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>
#include <QtGlobal>

namespace Restaurant {

FloorMapWidget::FloorMapWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(680, 490);
    setMouseTracking(true);
}

void FloorMapWidget::updateFloorData(const std::vector<TableInfo>& tables,
                                     const QMap<int, int>& occupancyByTableId,
                                     const QMap<int, double>& ratingByTableId,
                                     const QMap<int, QString>& guestBadgeByTableId,
                                     const QMap<int, int>& freeHoursByTableId) {
    tables_ = tables;
    occupancy_ = occupancyByTableId;
    ratings_ = ratingByTableId;
    guestBadges_ = guestBadgeByTableId;
    freeHours_ = freeHoursByTableId;
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
    const double topMargin = 76.0;
    const double bottomMargin = 44.0;
    const double leftMargin = 22.0;
    const double rightMargin = 22.0;

    double availW = std::max(400.0, width() - leftMargin - rightMargin);
    double availH = std::max(300.0, height() - topMargin - bottomMargin);

    double cellW = availW / 4.0;
    double cellH = availH / 3.0;

    double padX = 14.0;
    double padY = 12.0;

    double x = leftMargin + t.gridX * cellW + padX;
    double y = topMargin + t.gridY * cellH + padY;
    double w = cellW - 2.0 * padX;
    double h = cellH - 2.0 * padY;
    return QRectF(x, y, w, h);
}

void FloorMapWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 1. Фон зала
    p.fillRect(rect(), QColor(22, 24, 34));

    // 2. Легенда статусов сверху
    QRectF legendRect(22, 8, width() - 44, 24);
    QFont legendFont = font();
    legendFont.setPointSize(8);
    legendFont.setBold(true);
    p.setFont(legendFont);
    p.setPen(QColor(210, 215, 225));
    p.drawText(legendRect, Qt::AlignCenter,
               "🟢 СВОБОДЕН (доступен для записи)     🟡 БРОНЬ (забронирован онлайн)     "
               "🔴 ЗАНЯТ (гости за столом)     ⭐ Желтая рамка = выбранный стол");

    // 3. Верхний архитектурный ориентир — Панорамные окна и Терраса
    QRectF northBanner(22, 36, width() - 44, 28);
    p.setPen(QPen(QColor(70, 130, 180), 1.5));
    p.setBrush(QColor(30, 42, 64));
    p.drawRoundedRect(northBanner, 6, 6);
    p.setPen(QColor(175, 215, 255));
    QFont bannerFont = font();
    bannerFont.setBold(true);
    bannerFont.setPointSize(9);
    p.setFont(bannerFont);
    p.drawText(northBanner, Qt::AlignCenter,
               "ПАНОРАМНЫЕ ОКНА С ВИДОМ НА НАБЕРЕЖНУЮ (W1–W2)   |   ЛЕТНЯЯ ТЕРРАСА С ПЛЕДАМИ (T1–T2)");

    // 4. Нижний архитектурный ориентир — Вход, Хостес и Открытая кухня
    QRectF southBanner(22, height() - 34, width() - 44, 24);
    p.setPen(QPen(QColor(110, 95, 70), 1.2));
    p.setBrush(QColor(42, 36, 32));
    p.drawRoundedRect(southBanner, 6, 6);
    p.setPen(QColor(225, 200, 150));
    p.drawText(southBanner, Qt::AlignCenter,
               "КАМИННЫЙ VIP-ЗАЛ (V1–V2)   |   ГЛАВНЫЙ ВХОД И СТОЙКА ХОСТЕС   |   ОТКРЫТАЯ КУХНЯ И БАР (B1–B2)");

    // 5. Отрисовка каждого столика с адаптивной версткой внутри ячейки
    tableRects_.clear();
    for (const auto& t : tables_) {
        QRectF r = computeTableRect(t);
        tableRects_[t.id] = r;

        int st = occupancy_.value(t.id, 0);
        QColor fillCol;
        QColor borderCol;
        QString statusText;

        if (st == 0) {
            fillCol = QColor(30, 72, 52);
            borderCol = QColor(46, 204, 113);
            int fHrs = freeHours_.value(t.id, 0);
            statusText = (fHrs >= 4) ? "СВОБОДЕН" : QString("СВОБ (%1 ч)").arg(fHrs);
        } else if (st == 1) {
            fillCol = QColor(88, 66, 24);
            borderCol = QColor(243, 156, 18);
            statusText = guestBadges_.value(t.id, "БРОНЬ");
        } else {
            fillCol = QColor(88, 32, 36);
            borderCol = QColor(231, 76, 60);
            statusText = guestBadges_.value(t.id, "ЗАНЯТ");
        }

        bool isSelected = (t.id == selectedTableId_);
        bool isHovered = (t.id == hoveredTableId_);

        if (isHovered) {
            fillCol = fillCol.lighter(118);
        }

        // Отрисовка диванов/стульев по краям стола
        p.setPen(Qt::NoPen);
        p.setBrush(t.hasSofa ? QColor(125, 105, 155, 185) : QColor(90, 100, 115, 165));
        if (t.hasSofa) {
            p.drawRoundedRect(QRectF(r.left() + 10, r.top() - 5, r.width() - 20, 4), 2, 2);
            p.drawRoundedRect(QRectF(r.left() + 10, r.bottom() + 1, r.width() - 20, 4), 2, 2);
        } else {
            p.drawEllipse(QPointF(r.center().x() - 18, r.top() - 3), 5, 3);
            p.drawEllipse(QPointF(r.center().x() + 18, r.top() - 3), 5, 3);
            p.drawEllipse(QPointF(r.center().x() - 18, r.bottom() + 3), 5, 3);
            p.drawEllipse(QPointF(r.center().x() + 18, r.bottom() + 3), 5, 3);
        }

        // Основной контур стола
        QPen tablePen(isSelected ? QColor(255, 215, 0) : borderCol, isSelected ? 3.2 : 1.8);
        p.setPen(tablePen);
        p.setBrush(fillCol);
        p.drawRoundedRect(r, 9, 9);

        double hStep = r.height() / 4.2;

        // Строка 1: Номер и код стола + вместимость
        QFont titleF = font();
        titleF.setBold(true);
        titleF.setPointSize(9);
        p.setFont(titleF);
        p.setPen(Qt::white);

        QRectF topRow(r.left() + 7, r.top() + 3, r.width() - 14, hStep);
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

        QRectF zoneRow(r.left() + 7, r.top() + 3 + hStep, r.width() - 14, hStep);
        p.drawText(zoneRow, Qt::AlignLeft | Qt::AlignVCenter, t.zone);

        double rt = ratings_.value(t.id, 0.0);
        if (rt > 0.0) {
            p.setPen(QColor(255, 215, 90));
            p.drawText(zoneRow, Qt::AlignRight | Qt::AlignVCenter,
                       QString("★ %1").arg(rt, 0, 'f', 1));
        }

        // Строка 3: Значки оснащения (Окно, Диван, 220V, Тихо)
        QStringList feats;
        if (t.nearWindow) feats << "Окно";
        if (t.hasSofa)    feats << "Диван";
        if (t.hasOutlet)  feats << "220V";
        if (t.isQuiet)    feats << "Тихо";
        if (feats.isEmpty()) feats << "Стандарт";

        p.setPen(QColor(175, 198, 220));
        QRectF featRow(r.left() + 7, r.top() + 3 + hStep * 2.0, r.width() - 14, hStep);
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
                QString tip = QString("<b>Столик #%1 [%2]</b> — %3<br>"
                                      "Вместимость: до %4 чел.<br>"
                                      "Запас свободного времени: %5 ч.<br>%6")
                              .arg(t.id).arg(t.code).arg(t.zone).arg(t.capacity).arg(fHrs).arg(t.description);
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
