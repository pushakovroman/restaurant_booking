#include "AnalyticsChartWidget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <algorithm>

namespace Restaurant {

AnalyticsChartWidget::AnalyticsChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(260);
}

void AnalyticsChartWidget::updateCharts(const QMap<int, int>& hourlyOccupied,
                                        int totalTables,
                                        const QMap<QString, double>& revenueByZone) {
    hourlyOccupied_ = hourlyOccupied;
    totalTables_ = totalTables;
    revenueByZone_ = revenueByZone;
    update();
}

QSize AnalyticsChartWidget::minimumSizeHint() const { return QSize(320, 260); }
QSize AnalyticsChartWidget::sizeHint() const { return QSize(420, 320); }

void AnalyticsChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor("#141620"));

    const QColor textColor("#ECEFF4");
    const QColor barColor("#3498DB");
    const QColor zoneColor("#27AE60");

    const int margin = 10;
    const QRect area = rect().adjusted(margin, margin, -margin, -margin);
    const int halfH = area.height() / 2;

    // ---------- Верхняя диаграмма: загрузка по часам ----------
    QRect top(area.left(), area.top(), area.width(), halfH - 6);
    p.setPen(textColor);
    p.drawText(top.left(), top.top() + 12, QString::fromUtf8("Загрузка зала по часам (занято столов)"));

    QRect plot = top.adjusted(24, 22, 0, -16);
    if (plot.height() > 10 && !hourlyOccupied_.isEmpty()) {
        const int maxVal = std::max(1, std::max(totalTables_, *std::max_element(hourlyOccupied_.begin(), hourlyOccupied_.end())));
        const int n = hourlyOccupied_.size();
        const double slot = static_cast<double>(plot.width()) / n;
        int i = 0;
        for (auto it = hourlyOccupied_.constBegin(); it != hourlyOccupied_.constEnd(); ++it, ++i) {
            const double h = plot.height() * (static_cast<double>(it.value()) / maxVal);
            QRectF bar(plot.left() + i * slot + slot * 0.15, plot.bottom() - h, slot * 0.7, h);
            p.setPen(Qt::NoPen);
            p.setBrush(barColor);
            p.drawRect(bar);
            p.setPen(textColor);
            QFont f = font(); f.setPointSize(7); p.setFont(f);
            p.drawText(QRectF(plot.left() + i * slot, plot.bottom() + 1, slot, 12),
                       Qt::AlignHCenter | Qt::AlignTop, QString::number(it.key()));
        }
        p.setPen(textColor);
        p.drawText(top.left(), plot.top() + 8, QString::number(maxVal));
        p.drawLine(plot.bottomLeft(), plot.bottomRight());
    } else {
        p.setPen(textColor);
        p.drawText(plot, Qt::AlignCenter, QString::fromUtf8("Нет данных"));
    }

    // ---------- Нижняя диаграмма: выручка по зонам ----------
    p.setFont(font());
    QRect bottom(area.left(), area.top() + halfH + 6, area.width(), halfH - 6);
    p.setPen(textColor);
    p.drawText(bottom.left(), bottom.top() + 12, QString::fromUtf8("Выручка по зонам (BYN)"));

    QRect list = bottom.adjusted(0, 20, 0, 0);
    if (!revenueByZone_.isEmpty()) {
        double maxRev = 0.0;
        for (double v : revenueByZone_) maxRev = std::max(maxRev, v);
        if (maxRev <= 0.0) maxRev = 1.0;

        const int rows = revenueByZone_.size();
        const int rowH = std::max(14, std::min(24, list.height() / std::max(1, rows)));
        const int labelW = std::min(140, list.width() / 3);
        int y = list.top();
        for (auto it = revenueByZone_.constBegin(); it != revenueByZone_.constEnd(); ++it) {
            if (y + rowH > list.bottom() + 1) break;
            p.setPen(textColor);
            const QString label = QFontMetrics(font()).elidedText(it.key(), Qt::ElideRight, labelW - 4);
            p.drawText(QRect(list.left(), y, labelW, rowH), Qt::AlignVCenter | Qt::AlignLeft, label);

            const int barMax = list.width() - labelW - 70;
            const int bw = static_cast<int>(barMax * (it.value() / maxRev));
            p.setPen(Qt::NoPen);
            p.setBrush(zoneColor);
            p.drawRect(list.left() + labelW, y + 3, std::max(0, bw), rowH - 6);

            p.setPen(textColor);
            p.drawText(QRect(list.left() + labelW + bw + 4, y, 66, rowH),
                       Qt::AlignVCenter | Qt::AlignLeft, QString::number(it.value(), 'f', 2));
            y += rowH;
        }
    } else {
        p.setPen(textColor);
        p.drawText(list, Qt::AlignCenter, QString::fromUtf8("Нет данных"));
    }
}

} // namespace Restaurant
