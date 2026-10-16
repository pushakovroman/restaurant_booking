#include "FloorMapWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>

namespace Restaurant {

FloorMapWidget::FloorMapWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(680, 480);
    setMouseTracking(true);
}

void FloorMapWidget::updateFloorData(const std::vector<TableInfo>& tables,
                                     const QMap<int, int>& occupancyByTableId,
                                     const QMap<int, double>& ratingByTableId) {
    tables_ = tables;
    occupancy_ = occupancyByTableId;
    ratings_ = ratingByTableId;
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
    const double topMargin = 58.0;
    const double bottomMargin = 46.0;
    const double leftMargin = 24.0;
    const double rightMargin = 24.0;

    double availW = std::max(400.0, width() - leftMargin - rightMargin);
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

void FloorMapWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 1. Фон зала
    p.fillRect(rect(), QColor(24, 26, 36));

    // 2. Верхний архитектурный ориентир — Панорамные окна и Терраса
    QRectF northBanner(24, 12, width() - 48, 32);
    p.setPen(QPen(QColor(70, 130, 180), 1.5));
    p.setBrush(QColor(32, 44, 66));
    p.drawRoundedRect(northBanner, 6, 6);
    p.setPen(QColor(175, 215, 255));
    QFont bannerFont = font();
    bannerFont.setBold(true);
    bannerFont.setPointSize(9);
    p.setFont(bannerFont);
    p.drawText(northBanner, Qt::AlignCenter,
               "ПАНОРАМНОЕ ОСТЕКЛЕНИЕ (ВИД НА НАБЕРЕЖНУЮ)   |   ЛЕТНЯЯ ВЕРАНДА С ПЛЕДАМИ");

    // 3. Нижний архитектурный ориентир — Вход, Хостес и Открытая кухня
    QRectF southBanner(24, height() - 36, width() - 48, 26);
    p.setPen(QPen(QColor(110, 95, 70), 1.2));
    p.setBrush(QColor(42, 36, 32));
    p.drawRoundedRect(southBanner, 6, 6);
    p.setPen(QColor(225, 200, 150));
    p.drawText(southBanner, Qt::AlignCenter,
               "ГЛАВНЫЙ ВХОД И СТОЙКА ХОСТЕС          |          ОТКРЫТАЯ КУХНЯ И КОНТАКТНЫЙ БАР");

    // 4. Отрисовка каждого столика
    tableRects_.clear();
    for (const auto& t : tables_) {
        QRectF r = computeTableRect(t);
        tableRects_[t.id] = r;

        int st = occupancy_.value(t.id, 0);
        QColor fillCol;
        QColor borderCol;
        QString statusText;

        if (st == 0) {
            fillCol = QColor(33, 74, 54);
            borderCol = QColor(46, 204, 113);
            statusText = "СВОБОДЕН";
        } else if (st == 1) {
            fillCol = QColor(88, 66, 26);
            borderCol = QColor(243, 156, 18);
            statusText = "БРОНЬ";
        } else {
            fillCol = QColor(88, 34, 38);
            borderCol = QColor(231, 76, 60);
            statusText = "ЗАНЯТ";
        }

        bool isSelected = (t.id == selectedTableId_);
        bool isHovered = (t.id == hoveredTableId_);

        if (isHovered) {
            fillCol = fillCol.lighter(118);
        }

        // Отрисовка диванов/стульев по краям стола
        p.setPen(Qt::NoPen);
        p.setBrush(t.hasSofa ? QColor(120, 100, 150, 180) : QColor(90, 100, 115, 160));
        if (t.hasSofa) {
            p.drawRoundedRect(QRectF(r.left() + 10, r.top() - 6, r.width() - 20, 5), 2, 2);
            p.drawRoundedRect(QRectF(r.left() + 10, r.bottom() + 1, r.width() - 20, 5), 2, 2);
        } else {
            p.drawEllipse(QPointF(r.center().x() - 18, r.top() - 4), 5, 3);
            p.drawEllipse(QPointF(r.center().x() + 18, r.top() - 4), 5, 3);
            p.drawEllipse(QPointF(r.center().x() - 18, r.bottom() + 4), 5, 3);
            p.drawEllipse(QPointF(r.center().x() + 18, r.bottom() + 4), 5, 3);
        }

        // Основной контур стола
        QPen tablePen(isSelected ? QColor(255, 215, 0) : borderCol, isSelected ? 3.2 : 1.8);
        p.setPen(tablePen);
        p.setBrush(fillCol);
        p.drawRoundedRect(r, 10, 10);

        // Заголовок: Номер и код стола + вместимость
        QFont titleF = font();
        titleF.setBold(true);
        titleF.setPointSize(10);
        p.setFont(titleF);
        p.setPen(Qt::white);

        QRectF topRow(r.left() + 8, r.top() + 6, r.width() - 16, 20);
        p.drawText(topRow, Qt::AlignLeft | Qt::AlignVCenter,
                   QString("#%1 [%2]").arg(t.id).arg(t.code));
        p.drawText(topRow, Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1 чел.").arg(t.capacity));

        // Зона и рейтинг
        QFont subF = font();
        subF.setPointSize(8);
        subF.setBold(false);
        p.setFont(subF);
        p.setPen(QColor(210, 215, 225));

        QRectF zoneRow(r.left() + 8, r.top() + 26, r.width() - 16, 16);
        p.drawText(zoneRow, Qt::AlignLeft | Qt::AlignVCenter, t.zone);

        double rt = ratings_.value(t.id, 0.0);
        if (rt > 0.0) {
            p.setPen(QColor(255, 215, 90));
            p.drawText(zoneRow, Qt::AlignRight | Qt::AlignVCenter,
                       QString("★ %1").arg(rt, 0, 'f', 1));
        }

        // Значки оснащения (Окно, Диван, 220V, Тихо)
        QStringList feats;
        if (t.nearWindow) feats << "Окно";
        if (t.hasSofa)    feats << "Диван";
        if (t.hasOutlet)  feats << "220V";
        if (t.isQuiet)    feats << "Тихо";

        p.setPen(QColor(175, 195, 215));
        QRectF featRow(r.left() + 8, r.top() + 44, r.width() - 16, 16);
        p.drawText(featRow, Qt::AlignLeft | Qt::AlignVCenter, feats.join(" • "));

        // Плашка статуса внизу карточки стола
        QRectF badgeRect(r.left() + 10, r.bottom() - 24, r.width() - 20, 18);
        p.setPen(Qt::NoPen);
        p.setBrush(borderCol);
        p.drawRoundedRect(badgeRect, 4, 4);

        QFont badgeFont = font();
        badgeFont.setBold(true);
        badgeFont.setPointSize(8);
        p.setFont(badgeFont);
        p.setPen(QColor(20, 20, 25));
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
                QString tip = QString("<b>Столик #%1 [%2]</b> — %3<br>"
                                      "Вместимость: до %4 чел.<br>%5")
                              .arg(t.id).arg(t.code).arg(t.zone).arg(t.capacity).arg(t.description);
                QToolTip::showText(event->globalPos(), tip, this);
                break;
            }
        }
    }
    QWidget::mouseMoveEvent(event);
}

} // namespace Restaurant
