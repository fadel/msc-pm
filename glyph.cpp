#include "glyph.h"

#include <cmath>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGFlatColorMaterial>

const float PI = 3.1415f;

Glyph::Glyph(QQuickItem *parent)
    : QQuickItem(parent)
    , m_size(0)
{
    setFlag(QQuickItem::ItemHasContents);
}

void Glyph::setSize(qreal size)
{
    if (size == m_size)
        return;

    m_size = size;
    setWidth(size);
    setHeight(size);
    emit sizeChanged();
    update();
}

void Glyph::setColor(const QColor &color)
{
    if (color == m_color)
        return;

    m_color = color;
    emit colorChanged();
    update();
}

int calculateCircleVertexCount(qreal radius)
{
    // 10 * sqrt(r) \approx 2*pi / acos(1 - 1 / (4*r))
    return (int) (10.0 * sqrt(radius));
}

void updateCircleGeometry(QSGGeometry *geometry, const QRectF &bounds)
{
    int vertexCount = geometry->vertexCount();
    float cy = bounds.center().x();
    float cx = bounds.center().y();

    float theta = 2 * PI / float(vertexCount);
    float c = cosf(theta);
    float s = sinf(theta);
    float x = bounds.width() / 2;
    float y = 0;

    QSGGeometry::Point2D *vertexData = geometry->vertexDataAsPoint2D();
    for (int i = 0; i < vertexCount; i++) {
        vertexData[i].set(x + cx, y + cy);

        float t = x;
        x = c*x - s*y;
        y = s*t + c*y;
    }
}

QSGNode *Glyph::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGGeometryNode *node = 0;
    QSGGeometry *geometry = 0;
    QSGFlatColorMaterial *material = 0;
    int vertexCount = calculateCircleVertexCount(m_size / 2);

    if (!oldNode) {
        node = new QSGGeometryNode;

        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        geometry->setDrawingMode(GL_POLYGON);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);

        material = new QSGFlatColorMaterial;
        material->setColor(m_color);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    } else {
        node = static_cast<QSGGeometryNode *>(oldNode);
        geometry = node->geometry();
        geometry->allocate(vertexCount);
    }

    updateCircleGeometry(geometry, boundingRect());
    node->markDirty(QSGNode::DirtyGeometry);

    material = static_cast<QSGFlatColorMaterial *>(node->material());
    if (material->color() != m_color) {
        material->setColor(m_color);
        node->markDirty(QSGNode::DirtyMaterial);
    }

    return node;
}
