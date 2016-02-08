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
public:
    static const int PADDING = 20;

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
    void setSelection(const std::vector<bool> &selection);
    void brushItem(int item);
    void setScale(const LinearScale<float> &sx, const LinearScale<float> &sy);
    Q_INVOKABLE void setGlyphSize(float glyphSize);

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
    ColorScale m_colorScale;

    void autoScale();
    bool m_autoScale;
    LinearScale<float> m_sx, m_sy;

    // Internal state
    bool interactiveSelection(bool mergeSelection);
    std::vector<bool> m_selection;
    int m_brushedItem;

    enum State {
        STATE_NONE,
        STATE_SELECTING,
        STATE_SELECTED,
        STATE_BEGIN_MOVING,
        STATE_MOVING
    } m_interactionState;
    bool m_dragEnabled;

    QPointF m_dragOriginPos, m_dragCurrentPos;

    bool m_shouldUpdateGeometry, m_shouldUpdateMaterials;

    void updateQuadTree();
    QuadTree *m_quadtree;
};

#endif // SCATTERPLOT_H
