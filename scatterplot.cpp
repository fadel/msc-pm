#include "scatterplot.h"

#include <iostream>

#include <cmath>
#include <QSGNode>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGFlatColorMaterial>
#include <QSGSimpleRectNode>

const int GLYPH_SIZE = 5;
const float PADDING = 10;
const float PI = 3.1415f;

Scatterplot::Scatterplot(QQuickItem *parent)
    : QQuickItem(parent)
    , m_dragOriginPos(-1.0, -1.0)
    , m_colorScale{
        QColor("#1f77b4"),
        QColor("#ff7f0e"),
        QColor("#2ca02c"),
        QColor("#d62728"),
        QColor("#9467bd"),
        QColor("#8c564b"),
        QColor("#e377c2"),
        QColor("#7f7f7f"),
        QColor("#bcbd22"),
    }
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

Scatterplot::~Scatterplot()
{
}

void Scatterplot::setData(const arma::mat &data)
{
    if (data.n_cols != 3)
        return;

    m_data = data;
    m_colorScale.setExtents(m_data.col(2).min(), m_data.col(2).max());
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

void updateSelectionGeometry(QSGGeometry *geometry, const QPointF &p1, const QPointF &p2)
{
    QSGGeometry::Point2D *vertexData = geometry->vertexDataAsPoint2D();
    vertexData[0].set(p1.x(), p1.y());
    vertexData[1].set(p2.x(), p1.y());
    vertexData[2].set(p2.x(), p2.y());
    vertexData[3].set(p1.x(), p2.y());
}

QSGNode *Scatterplot::newGlyphNodeTree() {
    QSGNode *node = new QSGNode;
    int vertexCount = calculateCircleVertexCount(GLYPH_SIZE / 2);

    for (arma::uword i = 0; i < m_data.n_rows; i++) {
        QSGGeometryNode *glyphNode = new QSGGeometryNode;

        QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        geometry->setDrawingMode(GL_POLYGON);
        glyphNode->setGeometry(geometry);
        glyphNode->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(m_colorScale.color(m_data(i, 2)));
        glyphNode->setMaterial(material);
        glyphNode->setFlag(QSGNode::OwnsMaterial);

        node->appendChildNode(glyphNode);
    }

    return node;
}

QSGNode *Scatterplot::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_data.n_rows < 1)
        return 0;

    qreal xmin = m_data.col(0).min(),
          xmax = m_data.col(0).max(),
          ymin = m_data.col(1).min(),
          ymax = m_data.col(1).max(),
          x, y;

    QSGNode *root = 0;
    if (!oldNode) {
        root = new QSGNode;
        root->appendChildNode(newGlyphNodeTree());
    } else {
        root = oldNode;
    }

    QSGNode *glyphNode = root->firstChild()->firstChild();
    for (arma::uword i = 0; i < m_data.n_rows; i++) {
        arma::rowvec row = m_data.row(i);
        x = PADDING + (row[0] - xmin) / (xmax - xmin) * (width() - 2*PADDING);
        y = PADDING + (row[1] - ymin) / (ymax - ymin) * (height() - 2*PADDING);

        QSGGeometry *geometry = static_cast<QSGGeometryNode *>(glyphNode)->geometry();
        updateCircleGeometry(geometry, GLYPH_SIZE, x, y);
        glyphNode->markDirty(QSGNode::DirtyGeometry);
        glyphNode = glyphNode->nextSibling();
    }

    // Draw selection
    if (m_currentState == INTERACTION_SELECTING) {
        QSGGeometryNode *selectionNode = 0;
        if (!root->firstChild()->nextSibling()) {
            selectionNode = new QSGGeometryNode;
            QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
            geometry->setDrawingMode(GL_POLYGON);
            selectionNode->setGeometry(geometry);
            selectionNode->setFlag(QSGNode::OwnsGeometry);

            QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
            material->setColor(QColor(128, 128, 128, 128));
            selectionNode->setMaterial(material);
            selectionNode->setFlag(QSGNode::OwnsMaterial);

            root->appendChildNode(selectionNode);
        } else {
            selectionNode = static_cast<QSGGeometryNode *>(root->firstChild()->nextSibling());
        }

        updateSelectionGeometry(selectionNode->geometry(), m_dragOriginPos, m_dragCurrentPos);
        selectionNode->markDirty(QSGNode::DirtyGeometry);
    } else if (m_currentState == INTERACTION_NONE) {
        if (root->firstChild()->nextSibling()) {
            root->firstChild()->nextSibling()->markDirty(QSGNode::DirtyGeometry);
            root->removeChildNode(root->firstChild()->nextSibling());
        }
    }

    return root;
}

void Scatterplot::mousePressEvent(QMouseEvent *event)
{
    switch (m_currentState) {
    case INTERACTION_NONE:
        m_currentState = INTERACTION_SELECTING;
        m_dragOriginPos = event->localPos();
        break;
    case INTERACTION_SELECTED:
        m_currentState = INTERACTION_MOVING;
        break; // TODO
    case INTERACTION_SELECTING:
    case INTERACTION_MOVING:
        return; // should not be reached
    }
}

void Scatterplot::mouseMoveEvent(QMouseEvent *event)
{
    if (m_currentState != INTERACTION_SELECTING)
        return;

    m_dragCurrentPos = event->localPos();
    update();
}

void Scatterplot::mouseReleaseEvent(QMouseEvent *event)
{
    switch (m_currentState) {
    case INTERACTION_SELECTING:
        m_currentState = INTERACTION_NONE;
        update();
        break;

    case INTERACTION_MOVING:
        break;
    case INTERACTION_NONE:
    case INTERACTION_SELECTED:
        return; // should not be reached
    }
}
