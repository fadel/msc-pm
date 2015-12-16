#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include <memory>
#include <armadillo>
#include <QtQuick>
#include <QSet>
#include <QEasingCurve>

#include "colorscale.h"
#include "scale.h"

class Scatterplot : public QQuickItem
{
    Q_OBJECT
public:
    Scatterplot(QQuickItem *parent = 0);

    arma::mat XY() const;
    void setColorScale(ColorScale *colorScale);
    Q_INVOKABLE bool saveToFile(const QUrl &url);

signals:
    void xyChanged(const arma::mat &XY) const;
    void xyInteractivelyChanged(const arma::mat &XY) const;
    void colorDataChanged(const arma::vec &colorData) const;
    void selectionChanged(const QSet<int> &selection) const;

public slots:
    void setXY(const arma::mat &xy);
    void setColorData(const arma::vec &colorData);
    void setSelection(const QSet<int> &selection);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QSGNode *createGlyphNodeTree();
    bool updateSelection(bool mergeSelection);

    void applyManipulation();

    void updateGeometry();
    void updateMaterials();

    void resetAnimation();
    void startAnimation();
    void animationTick();

    arma::mat m_oldXY, m_xy;
    LinearScale<float> m_sx, m_sy;

    enum InteractionState {
        INTERACTION_NONE,
        INTERACTION_SELECTING,
        INTERACTION_SELECTED,
        INTERACTION_BEGIN_MOVING,
        INTERACTION_MOVING
    } m_currentInteractionState;

    QPointF m_dragOriginPos, m_dragCurrentPos;

    QSet<int> m_selectedGlyphs;

    bool m_shouldUpdateGeometry, m_shouldUpdateMaterials;

    arma::vec m_colorData;
    ColorScale *m_colorScale;

    QEasingCurve m_animationEasing;
    float m_t;
};

#endif // SCATTERPLOT_H
