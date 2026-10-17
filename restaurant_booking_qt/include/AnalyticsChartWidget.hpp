#ifndef ANALYTICS_CHART_WIDGET_HPP
#define ANALYTICS_CHART_WIDGET_HPP

#include <QWidget>
#include <QMap>
#include <QString>

namespace Restaurant {

// Рисует две диаграммы: почасовая загрузка зала и выручка по зонам.
class AnalyticsChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnalyticsChartWidget(QWidget* parent = nullptr);

    void updateCharts(const QMap<int, int>& hourlyOccupied,
                      int totalTables,
                      const QMap<QString, double>& revenueByZone);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QMap<int, int> hourlyOccupied_;
    int totalTables_ = 0;
    QMap<QString, double> revenueByZone_;
};

} // namespace Restaurant

#endif // ANALYTICS_CHART_WIDGET_HPP
