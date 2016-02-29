#ifndef COLORMAP_H
#define COLORMAP_H

#include <vector>

#include <QtQuick>
#include <QSGDynamicTexture>

#include <armadillo>

#include "colorscale.h"

class Colormap
    : public QQuickItem
{
    Q_OBJECT
    Q_ENUMS(Orientation)
    Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
public:
    static const int SAMPLES = 128;

    enum Orientation {
        Horizontal,
        Vertical
    };

    Colormap(QQuickItem *parent = 0);
    ~Colormap();

    void setOrientation(Orientation orientation);
    Orientation orientation() const { return m_orientation; }

signals:
    void colorScaleChanged(const ColorScale &scale) const;
    void orientationChanged(Orientation orientation) const;

public slots:
    void setColorScale(const ColorScale &scale);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

private:
    QSGNode *newSceneGraph();

    QSGDynamicTexture *m_texture;
    bool m_shouldUpdateTexture, m_shouldUpdateOrientation;
    Orientation m_orientation;
    std::vector<float> m_cmap;
};

#endif // COLORMAP_H
