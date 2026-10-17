#ifndef FLOOR_MAP_WIDGET_HPP
#define FLOOR_MAP_WIDGET_HPP

#include "Models.hpp"
#include <QWidget>
#include <QMap>
#include <QRectF>
#include <vector>

namespace Restaurant {

class FloorMapWidget : public QWidget {
    Q_OBJECT

public:
    explicit FloorMapWidget(QWidget* parent = nullptr);

    void updateFloorData(const std::vector<TableInfo>& tables,
                         const QMap<int, int>& occupancyByTableId,
                         const QMap<int, double>& ratingByTableId);

    int selectedTableId() const { return selectedTableId_; }
    void setSelectedTableId(int tableId);

signals:
    void tableSelected(int tableId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    std::vector<TableInfo> tables_;
    QMap<int, int> occupancy_;     // 0 = Свободен, 1 = Бронь, 2 = Занят
    QMap<int, double> ratings_;
    QMap<int, QRectF> tableRects_;
    int selectedTableId_ = 1;
    int hoveredTableId_ = 0;

    QRectF computeTableRect(const TableInfo& t) const;
};

} // namespace Restaurant

#endif // FLOOR_MAP_WIDGET_HPP
