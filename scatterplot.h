#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include <armadillo>
#include <QtQuick>

#include "colorscale.h"

class Scatterplot : public QQuickItem
{
    Q_OBJECT
public:
    Scatterplot(QQuickItem *parent = 0);

    void setColorScale(const ColorScale &colorScale);

signals:
    void xyChanged(const arma::mat &XY);
    void colorDataChanged(const arma::vec &colorData);

public slots:
    void setXY(const arma::mat &xy);
    void setColorData(const arma::vec &colorData);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QSGNode *createGlyphNodeTree();
    bool selectGlyphs(bool mergeSelection);

    float fromDataXToScreenX(float x);
    float fromDataYToScreenY(float y);

    void applyManipulation();

    void updateGeometry();
    void updateMaterials();

    arma::mat m_xy;
    float m_xmin, m_xmax, m_ymin, m_ymax;

    bool m_shouldUpdateGeometry, m_shouldUpdateMaterials;

    arma::vec m_colorData;

    ColorScale m_colorScale;

    enum InteractionState {
        INTERACTION_NONE,
        INTERACTION_SELECTING,
        INTERACTION_SELECTED,
        INTERACTION_MOVING
    } m_currentState;
    QPointF m_dragOriginPos, m_dragCurrentPos;

    QSet<int> m_selectedGlyphs;
};

#endif // SCATTERPLOT_H
