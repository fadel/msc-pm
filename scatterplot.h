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
    void setXY(const arma::mat &xy, bool updateView);
    void setColorData(const arma::vec &colorData, bool updateView);
    void setOpacityData(const arma::vec &opacityData, bool updateView);
    void setScale(const LinearScale<float> &sx, const LinearScale<float> &sy, bool updateView);
    void setAutoScale(bool autoScale);
    Q_INVOKABLE bool saveToFile(const QUrl &url);

signals:
    void xyChanged(const arma::mat &XY) const;
    void xyInteractivelyChanged(const arma::mat &XY) const;
    void colorDataChanged(const arma::vec &colorData) const;
    void opacityDataChanged(const arma::vec &opacityData) const;
    void selectionChanged(const QSet<int> &selection) const;
    void displaySplatChanged(bool displaySplat) const;
    void scaleChanged(const LinearScale<float> &sx, const LinearScale<float> &sy) const;

public slots:
    void setXY(const arma::mat &xy);
    void setColorData(const arma::vec &colorData);
    void setOpacityData(const arma::vec &opacityData);
    void setSelection(const QSet<int> &selection);
    void setDisplaySplat(bool displaySplat);
    void setScale(const LinearScale<float> &sx, const LinearScale<float> &sy);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QSGNode *newSceneGraph();
    QSGNode *newSplatNode();
    QSGNode *newGlyphTree();
    bool updateSelection(bool mergeSelection);

    void applyManipulation();

    void updateGeometry();
    void updateMaterials();

    void updateSplat(QSGNode *node);
    void updateGlyphs(QSGNode *node);

    arma::mat m_xy;

    void autoScale();
    bool m_autoScale;
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

    bool m_displaySplat;
    ColorScale *m_colorScale;

    arma::vec m_colorData;
    arma::vec m_opacityData;
};

#endif // SCATTERPLOT_H
