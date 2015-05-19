#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include <armadillo>
#include <vector>
#include <QQuickItem>

#include "colorscale.h"

class Scatterplot : public QQuickItem
{
    Q_OBJECT
public:
    Scatterplot(QQuickItem *parent = 0);
    ~Scatterplot();

    void setData(const arma::mat &data);

signals:

public slots:

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QSGNode *newGlyphNodeTree();

    enum InteractionState {
        INTERACTION_NONE,
        INTERACTION_SELECTING,
        INTERACTION_SELECTED,
        INTERACTION_MOVING
    } m_currentState;
    QPointF m_dragOriginPos, m_dragCurrentPos;

    arma::mat m_data;
    ColorScale m_colorScale;
};

#endif // SCATTERPLOT_H
