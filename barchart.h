#ifndef BARCHART_H
#define BARCHART_H

#include <QtQuick>
#include <armadillo>

class BarChart : public QQuickItem
{
    Q_OBJECT
public:
    BarChart(QQuickItem *parent = 0);
    ~BarChart();

signals:
    void valuesChanged(const arma::vec &values) const;

public slots:
    void setValues(const arma::vec &values);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void hoverMoveEvent(QHoverEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    QSGNode *newBarNode() const;
    void updateBarNodeGeom(QSGNode *barNode, float x, float width, float height);
    void updateBars(QSGNode *root);

    arma::vec m_values;
    bool m_shouldUpdateBars;
};

#endif // BARCHART_H
