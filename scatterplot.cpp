#include "scatterplot.h"

#include <cmath>
#include <cstdio>
#include <QSGNode>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGFlatColorMaterial>

const int GLYPH_SIZE = 4;
const float PADDING = 5;
const float PI = 3.1415f;

Scatterplot::Scatterplot()
{
    setFlag(QQuickItem::ItemHasContents);
}

Scatterplot::~Scatterplot()
{
}

void Scatterplot::setData(const arma::mat &data)
{
    if (data.n_cols != 2)
        return;

    m_data = data;
    update();
}

int calculateCircleVertexCount(qreal radius)
{
    // 10 * sqrt(r) \approx 2*pi / acos(1 - 1 / (4*r))
    return (int) (10.0 * sqrt(radius));
}

void updateCircleGeometry(QSGGeometry *geometry, float size, float cx, float cy)
{
    int vertexCount = geometry->vertexCount();

    float theta = 2 * PI / float(vertexCount);
    float c = cosf(theta);
    float s = sinf(theta);
    float x = size / 2;
    float y = 0;

    QSGGeometry::Point2D *vertexData = geometry->vertexDataAsPoint2D();
    for (int i = 0; i < vertexCount; i++) {
        vertexData[i].set(x + cx, y + cy);

        float t = x;
        x = c*x - s*y;
        y = s*t + c*y;
    }
}

void updateSquareGeometry(QSGGeometry *geometry, float size, float cx, float cy)
{
    float r = size / 2;
    QSGGeometry::Point2D *vertexData = geometry->vertexDataAsPoint2D();
    vertexData[0].set(cx - r, cy - r);
    vertexData[1].set(cx + r, cy - r);
    vertexData[2].set(cx + r, cy + r);
    vertexData[3].set(cx - r, cy + r);
}

QSGNode *Scatterplot::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *node = 0;
    QSGGeometryNode *childNode = 0;
    QSGGeometry *geometry = 0;
    QSGFlatColorMaterial *material = 0;
    int vertexCount = calculateCircleVertexCount(GLYPH_SIZE / 2);
    qreal xmin = m_data.col(0).min(),
          xmax = m_data.col(0).max(),
          ymin = m_data.col(1).min(),
          ymax = m_data.col(1).max(),
          x, y;

    if (!oldNode) {
        node = new QSGNode;
        for (arma::uword i = 0; i < m_data.n_rows; i++) {
            arma::rowvec row = m_data.row(i);
            x = (row[0] - xmin) / (xmax - xmin) * width();
            y = (row[1] - ymin) / (ymax - ymin) * height();

            childNode = new QSGGeometryNode;

            geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
            geometry->setDrawingMode(GL_POLYGON);
            childNode->setGeometry(geometry);
            childNode->setFlag(QSGNode::OwnsGeometry);

            material = new QSGFlatColorMaterial;
            material->setColor(QColor());
            childNode->setMaterial(material);
            childNode->setFlag(QSGNode::OwnsMaterial);

            node->appendChildNode(childNode);
        }
    } else {
        node = oldNode;
    }

    childNode = static_cast<QSGGeometryNode *>(node->firstChild());
    for (arma::uword i = 0; i < m_data.n_rows; i++) {
        arma::rowvec row = m_data.row(i);
        x = (row[0] - xmin) / (xmax - xmin) * width();
        y = (row[1] - ymin) / (ymax - ymin) * height();

        geometry = childNode->geometry();
        updateCircleGeometry(geometry, GLYPH_SIZE, x, y);
        childNode = static_cast<QSGGeometryNode *>(childNode->nextSibling());
    }
    node->markDirty(QSGNode::DirtyGeometry);

    return node;
}
