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
    void colorScaleChanged(const ColorScale &scale) const;

public slots:
    void setValues(const arma::vec &values);
    void setColorScale(const ColorScale &scale);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

    void hoverEnterEvent(QHoverEvent *event);
    void hoverMoveEvent(QHoverEvent *event);
    void hoverLeaveEvent(QHoverEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QSGNode *newSceneGraph() const;
    QSGNode *newBarNode() const;

    void updateViewport(QSGNode *root) const;
    void updateBarNodeGeom(QSGNode *barNode, float x, float width, float height);
    void updateBarNodeColor(QSGNode *barNode, const QColor &color);
    void updateBars(QSGNode *node);
    void updateHoverHints(QSGNode *node);
    bool m_shouldUpdateBars;
    float m_hoverPos;

    arma::vec m_values;
    ColorScale m_colorScale;
    std::vector<int> m_originalIndices;
    LinearScale<float> m_scale;
};

#endif // BARCHART_H
