#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include <QtQuick>
#include <QSet>

#include <armadillo>

#include "colorscale.h"
#include "scale.h"

class Scatterplot
    : public QQuickItem
{
    Q_OBJECT
public:
    Scatterplot(QQuickItem *parent = 0);

    arma::mat XY() const;
    void setColorScale(const ColorScale &colorScale);
    void setXY(const arma::mat &xy, bool updateView);
    void setColorData(const arma::vec &colorData, bool updateView);
    void setOpacityData(const arma::vec &opacityData, bool updateView);
    void setScale(const LinearScale<float> &sx, const LinearScale<float> &sy, bool updateView);
    void setGlyphSize(float glyphSize, bool updateView);
    void setAutoScale(bool autoScale);
    Q_INVOKABLE bool saveToFile(const QUrl &url);

    Q_INVOKABLE float glyphSize() const { return m_glyphSize; }

    static const int PADDING = 20;

signals:
    void xyChanged(const arma::mat &XY) const;
    void xyInteractivelyChanged(const arma::mat &XY) const;
    void colorDataChanged(const arma::vec &colorData) const;
    void opacityDataChanged(const arma::vec &opacityData) const;
    void selectionChanged(const QSet<int> &selection) const;
    void scaleChanged(const LinearScale<float> &sx, const LinearScale<float> &sy) const;
    void glyphSizeChanged(float glyphSize) const;

public slots:
    void setXY(const arma::mat &xy);
    void setColorData(const arma::vec &colorData);
    void setOpacityData(const arma::vec &opacityData);
    void setSelection(const QSet<int> &selection);
    void setScale(const LinearScale<float> &sx, const LinearScale<float> &sy);
    Q_INVOKABLE void setGlyphSize(float glyphSize);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QSGNode *newSceneGraph();
    QSGNode *newGlyphTree();

    void applyManipulation();
    void updateGlyphs(QSGNode *node);

    // Data
    arma::mat m_xy;
    arma::vec m_colorData;
    arma::vec m_opacityData;

    // Visuals
    float m_glyphSize;
    ColorScale m_colorScale;

    void autoScale();
    bool m_autoScale;
    LinearScale<float> m_sx, m_sy;

    // Internal state
    bool updateSelection(bool mergeSelection);
    QSet<int> m_selectedGlyphs;

    enum InteractionState {
        INTERACTION_NONE,
        INTERACTION_SELECTING,
        INTERACTION_SELECTED,
        INTERACTION_BEGIN_MOVING,
        INTERACTION_MOVING
    } m_currentInteractionState;

    QPointF m_dragOriginPos, m_dragCurrentPos;

    bool m_shouldUpdateGeometry, m_shouldUpdateMaterials;
};

#endif // SCATTERPLOT_H
