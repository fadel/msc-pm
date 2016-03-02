#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include <vector>

#include <QtQuick>

#include <armadillo>

#include "colorscale.h"
#include "scale.h"

class QuadTree;

class Scatterplot
    : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(float glyphSize READ glyphSize WRITE setGlyphSize NOTIFY glyphSizeChanged)
public:
    static const int PADDING = 20;

    Scatterplot(QQuickItem *parent = 0);

    arma::mat XY() const;
    void setColorScale(const ColorScale *colorScale);
    void setAutoScale(bool autoScale);
    float glyphSize() const { return m_glyphSize; }
    void setGlyphSize(float glyphSize);

    void setDragEnabled(bool enabled) { m_dragEnabled = enabled; }
    bool isDragEnabled() const { return m_dragEnabled; }

signals:
    void xyChanged(const arma::mat &XY) const;
    void xyInteractivelyChanged(const arma::mat &XY) const;
    void colorDataChanged(const arma::vec &colorData) const;
    void opacityDataChanged(const arma::vec &opacityData) const;
    void selectionChanged(const std::vector<bool> &selection) const;
    void selectionInteractivelyChanged(const std::vector<bool> &selection) const;
    void itemBrushed(int item) const;
    void itemInteractivelyBrushed(int item) const;
    void scaleChanged(const LinearScale<float> &sx, const LinearScale<float> &sy) const;
    void glyphSizeChanged(float glyphSize) const;

public slots:
    void setXY(const arma::mat &xy);
    void setColorData(const arma::vec &colorData);
    void setOpacityData(const arma::vec &opacityData);
    void setScale(const LinearScale<float> &sx, const LinearScale<float> &sy);
    void setSelection(const std::vector<bool> &selection);
    void brushItem(int item);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    void hoverEnterEvent(QHoverEvent *event);
    void hoverMoveEvent(QHoverEvent *event);
    void hoverLeaveEvent(QHoverEvent *event);

private:
    QSGNode *newSceneGraph();
    QSGNode *newGlyphTree();

    void applyManipulation();
    void updateGlyphs(QSGNode *node);
    void updateBrush(QSGNode *node);

    // Data
    arma::mat m_xy;
    arma::vec m_colorData;
    arma::vec m_opacityData;

    // Visuals
    float m_glyphSize;
    const ColorScale *m_colorScale;

    void autoScale();
    bool m_autoScale;
    LinearScale<float> m_sx, m_sy;

    // Internal state
    void interactiveSelection(bool mergeSelection);
    std::vector<bool> m_selection;
    bool m_anySelected;
    int m_brushedItem;

    enum State {
        StateNone,
        StateBrushing,
        StateSelecting,
        StateSelected,
        StateMoving
    } m_interactionState;
    bool m_dragEnabled;

    QPointF m_dragOriginPos, m_dragCurrentPos;

    bool m_shouldUpdateGeometry, m_shouldUpdateMaterials;

    QuadTree *m_quadtree;
    void updateQuadTree();
};

#endif // SCATTERPLOT_H
