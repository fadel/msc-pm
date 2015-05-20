#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include <armadillo>
#include <QQuickItem>
#include <QSGNode>

#include "colorscale.h"

class Scatterplot : public QQuickItem
{
    Q_OBJECT
public:
    Scatterplot(QQuickItem *parent = 0);
    ~Scatterplot();

signals:
    void dataChanged(const arma::mat &data);

public slots:
    void setData(const arma::mat &data);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QSGNode *newGlyphNodeTree();
    bool selectGlyphs(bool mergeSelection);
    void updateData();

    enum InteractionState {
        INTERACTION_NONE,
        INTERACTION_SELECTING,
        INTERACTION_SELECTED,
        INTERACTION_MOVING
    } m_currentState;
    QPointF m_dragOriginPos, m_dragCurrentPos;
    QList<bool> m_selectedGlyphs;

    arma::mat m_data;
    ColorScale m_colorScale;
};

#endif // SCATTERPLOT_H
