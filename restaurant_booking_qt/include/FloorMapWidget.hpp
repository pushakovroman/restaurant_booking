#ifndef FLOOR_MAP_WIDGET_HPP
#define FLOOR_MAP_WIDGET_HPP

#include "Models.hpp"
#include <QWidget>
#include <QMap>
#include <QRectF>
#include <QPoint>
#include <QPointF>
#include <QTimer>
#include <vector>

namespace Restaurant {

class FloorMapWidget : public QWidget {
    Q_OBJECT

public:
    explicit FloorMapWidget(QWidget* parent = nullptr);

    void updateFloorData(const std::vector<TableInfo>& tables,
                         const QMap<int, int>& occupancyByTableId,
                         const QMap<int, double>& ratingByTableId,
                         const QMap<int, QString>& guestBadgeByTableId,
                         const QMap<int, int>& freeHoursByTableId,
                         const QMap<int, int>& selectedTableHourlyStatus);

    int selectedTableId() const { return selectedTableId_; }
    void setSelectedTableId(int tableId);

signals:
    void tableSelected(int tableId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    std::vector<TableInfo> tables_;
    QMap<int, int> occupancy_;     // 0 = Свободен, 1 = Бронь, 2 = Занят
    QMap<int, double> ratings_;
    QMap<int, QString> guestBadges_;
    QMap<int, int> freeHours_;
    QMap<int, int> selectedHourly_; // Час (10..22) -> статус (0, 1, 2)
    QMap<int, QRectF> tableRects_;
    int selectedTableId_ = 1;
    int hoveredTableId_ = 0;
    double pulsePhase_ = 0.0;
    QTimer* pulseTimer_ = nullptr;

    QRectF computeTableRect(const TableInfo& t) const;
    void drawArchitecturalDecor(QPainter& p) const;
    void drawChairsAndSofas(QPainter& p, const TableInfo& t, const QRectF& r) const;
    void drawBottomGanttStrip(QPainter& p) const;
};

} // namespace Restaurant

#endif // FLOOR_MAP_WIDGET_HPP
