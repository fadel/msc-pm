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
public:
    static const int SAMPLES = 128;

    Colormap(QQuickItem *parent = 0);
    ~Colormap();

signals:
    void colorScaleChanged(const ColorScale &scale) const;

public slots:
    void setColorScale(const ColorScale &scale);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

private:
    QSGNode *newSceneGraph();

    QSGDynamicTexture *m_texture;
    bool m_shouldUpdateTexture;
    std::vector<float> m_cmap;
};

#endif // COLORMAP_H
