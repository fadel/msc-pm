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
    void selectionChanged(const std::vector<bool> &selection) const;
    void selectionInteractivelyChanged(const std::vector<bool> &selection) const;
    void itemBrushed(int item) const;
    void itemInteractivelyBrushed(int item) const;

public slots:
    void setValues(const arma::vec &values);
    void setColorScale(const ColorScale &scale);
    void setSelection(const std::vector<bool> &selection);
    void brushItem(int item);

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
    QSGNode *newSelectionBarNode() const;

    void updateViewport(QSGNode *root) const;
    void updateBarNodeGeom(QSGNode *barNode, float x, float width, float height) const;
    void updateBarNodeColor(QSGNode *barNode, const QColor &color) const;
    void updateBars(QSGNode *node) const;
    bool m_shouldUpdateBars;

    void updatePreSelection(QSGNode *node) const;
    bool m_shouldUpdatePreSelection;
    float m_dragStartPos, m_dragLastPos;

    void updateSelectionBar(QSGNode *node, float x, float barWidth, const QColor &color) const;
    void updateSelectionBars(QSGNode *node) const;
    bool m_shouldUpdateSelection;

    void interactiveSelection(float start, float end);
    std::vector<bool> m_selection;

    void updateBrush(QSGNode *node) const;
    int m_brushedItem;

    int itemAt(float x, bool includeSelectorWidth = false) const;

    arma::vec m_values;
    ColorScale m_colorScale;
    std::vector<int> m_originalIndices, m_currentIndices;
    LinearScale<float> m_scale;
};

#endif // BARCHART_H
