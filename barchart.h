#ifndef BARCHART_H
#define BARCHART_H

#include <vector>

#include <QtQuick>

#include <armadillo>

#include "colorscale.h"
#include "scale.h"

class BarChart
    : public QQuickItem
{
    Q_OBJECT
public:
    BarChart(QQuickItem *parent = 0);
    ~BarChart();

signals:
    void valuesChanged(const arma::vec &values) const;
    void colorScaleChanged(const ColorScale *scale) const;

public slots:
    void setValues(const arma::vec &values);
    void setColorScale(const ColorScale *scale);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void hoverMoveEvent(QHoverEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    QSGNode *newBarNode() const;
    void updateBarNodeGeom(QSGNode *barNode, float x, float width, float height);
    void updateBarNodeColor(QSGNode *barNode, const QColor &color);
    void updateBars(QSGNode *root);
    bool m_shouldUpdateBars;

    arma::vec m_values;
    const ColorScale *m_colorScale;
    std::vector<int> m_originalIndices;
    LinearScale<float> m_scale;
};

#endif // BARCHART_H
