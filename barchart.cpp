#include "barchart.h"

#include <algorithm>

#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>

#include "continuouscolorscale.h"
#include "geometry.h"

static const QColor OUTLINE_COLOR(0, 0, 0);
static const QColor HINTS_COLOR(0, 0, 0);
static const QColor BAR_COLOR(128, 128, 128);
static const float DEFAULT_OPACITY = 0.8f;

BarChart::BarChart(QQuickItem *parent)
    : QQuickItem(parent)
    , m_shouldUpdateBars(false)
    , m_hoverPos(-1.0f)
    , m_colorScale(ContinuousColorScale::builtin(ContinuousColorScale::HEATED_OBJECTS))
    , m_scale(0.0f, 1.0f, 0.0f, 1.0f)
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
    //setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

BarChart::~BarChart()
{
}

void BarChart::setValues(const arma::vec &values)
{
    m_values = values;

    m_originalIndices.resize(m_values.n_elem);
    if (m_values.n_elem > 0) {
        m_scale.setDomain(m_values.min(), m_values.max());
        m_colorScale.setExtents(m_values.min(), m_values.max());

        for (int i = 0; i < m_originalIndices.size(); i++) {
            m_originalIndices[i] = i;
        }

        std::sort(m_originalIndices.begin(), m_originalIndices.end(),
            [this](int i, int j) { return m_values[i] > m_values[j]; });
    }
    emit valuesChanged(values);

    m_shouldUpdateBars = true;
    update();
}

void BarChart::setColorScale(const ColorScale &scale)
{
    m_colorScale = scale;
    if (m_values.n_elem > 0) {
        m_colorScale.setExtents(m_values.min(), m_values.max());
    }
    emit colorScaleChanged(m_colorScale);

    m_shouldUpdateBars = true;
    update();
}

QSGNode *BarChart::newSceneGraph() const
{
    // NOTE: scene graph structure is as follows:
    // root [ barsNode [ ... ] hoverHintsNode ]
    QSGTransformNode *root = new QSGTransformNode;

    // The node that has all bars as children
    root->appendChildNode(new QSGNode);

    // The node for drawing the hover hints
    QSGGeometryNode *hintsGeomNode = new QSGGeometryNode;
    QSGGeometry *hintsGeom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2);
    hintsGeom->setDrawingMode(GL_LINES);
    hintsGeom->setVertexDataPattern(QSGGeometry::DynamicPattern);
    hintsGeom->setLineWidth(1.0f);
    QSGFlatColorMaterial *hintsMaterial = new QSGFlatColorMaterial;
    hintsMaterial->setColor(HINTS_COLOR);
    hintsGeomNode->setGeometry(hintsGeom);
    hintsGeomNode->setMaterial(hintsMaterial);
    hintsGeomNode->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    root->appendChildNode(hintsGeomNode);

    return root;
}

QSGNode *BarChart::newBarNode() const
{
    // A bar node is:
    // opacityNode [outlineGeomNode barGeomNode]

    //QSGGeometryNode *outlineGeomNode = new QSGGeometryNode;
    //QSGGeometry *outlineGeometry =
    //    new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    //outlineGeometry->setDrawingMode(GL_LINE_LOOP);
    //outlineGeometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
    //outlineGeomNode->setGeometry(outlineGeometry);
    //outlineGeomNode->setFlag(QSGNode::OwnsGeometry);
    //QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
    //material->setColor(OUTLINE_COLOR);
    //outlineGeomNode->setMaterial(material);
    //outlineGeomNode->setFlag(QSGNode::OwnsMaterial);

    QSGGeometryNode *barGeomNode = new QSGGeometryNode;
    QSGGeometry *barGeometry =
        new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    barGeometry->setDrawingMode(GL_POLYGON);
    barGeometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
    barGeomNode->setGeometry(barGeometry);
    barGeomNode->setFlag(QSGNode::OwnsGeometry);
    QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
    material->setColor(BAR_COLOR);
    barGeomNode->setMaterial(material);
    barGeomNode->setFlag(QSGNode::OwnsMaterial);

    QSGOpacityNode *opacityNode = new QSGOpacityNode;
    opacityNode->setOpacity(DEFAULT_OPACITY);
    opacityNode->appendChildNode(barGeomNode);
    //opacityNode->appendChildNode(outlineGeomNode);

    return opacityNode;
}

void BarChart::updateViewport(QSGNode *root) const
{
    QSGTransformNode *viewportNode = static_cast<QSGTransformNode *>(root);
    QMatrix4x4 viewport;
    viewport.scale(width(), height());
    viewportNode->setMatrix(viewport);
}

void BarChart::updateBarNodeGeom(QSGNode *barNode, float x, float barWidth, float barHeight)
{
    float y = 1.0f - barHeight;

    QSGGeometryNode *barGeomNode =
        static_cast<QSGGeometryNode *>(barNode->firstChild());
    updateRectGeometry(barGeomNode->geometry(), x, y, barWidth, barHeight);
    barGeomNode->markDirty(QSGNode::DirtyGeometry);

    //QSGGeometryNode *outlineGeomNode =
    //    static_cast<QSGGeometryNode *>(barGeomNode->nextSibling());
    //updateRectGeometry(outlineGeomNode->geometry(), x, y, barWidth, barHeight);
    //outlineGeomNode->markDirty(QSGNode::DirtyGeometry);
}

void BarChart::updateBarNodeColor(QSGNode *barNode, const QColor &color)
{
    QSGGeometryNode *barGeomNode =
        static_cast<QSGGeometryNode *>(barNode->firstChild());

    QSGFlatColorMaterial *material =
        static_cast<QSGFlatColorMaterial *>(barGeomNode->material());
    material->setColor(color);
    barGeomNode->markDirty(QSGNode::DirtyMaterial);
}

void BarChart::updateBars(QSGNode *root)
{
    float barWidth = 1.0f / m_values.n_elem;

    QSGNode *node = root->firstChild();
    float x = 0;
    for (auto it = m_originalIndices.cbegin(); it != m_originalIndices.cend(); it++) {
        updateBarNodeGeom(node, x, barWidth, m_scale(m_values[*it]));
        updateBarNodeColor(node, m_colorScale.color(m_values[*it]));
        x += barWidth;
        node = node->nextSibling();
    }
}

void BarChart::updateHoverHints(QSGNode *node)
{
    QSGGeometryNode *hintsGeomNode = static_cast<QSGGeometryNode *>(node);
    QSGGeometry *hintsGeom = hintsGeomNode->geometry();

    QSGGeometry::Point2D *vertexData = hintsGeom->vertexDataAsPoint2D();
    vertexData[0].set(m_hoverPos, 0.0f);
    vertexData[1].set(m_hoverPos, 1.0f);

    hintsGeomNode->markDirty(QSGNode::DirtyGeometry);
}

QSGNode *BarChart::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode ? oldNode : newSceneGraph();
    updateViewport(root);

    QSGNode *node = root->firstChild();
    if (m_shouldUpdateBars) {
        int numValues = (int) m_values.n_elem;
        // First, make sure we have the same number of values & bars
        while (numValues > node->childCount()) {
            QSGNode *barNode = newBarNode();
            node->prependChildNode(barNode);
        }
        while (numValues < root->childCount()) {
            // NOTE: as stated in docs, QSGNode's children are stored in a
            // linked list. Hence, this operation should be as fast as expected
            node->removeChildNode(node->firstChild());
        }

        // Then, update the bars to reflect the values
        updateBars(node);
        m_shouldUpdateBars = false;
    }
    node = node->nextSibling();

    updateHoverHints(node);

    return root;
}

void BarChart::hoverEnterEvent(QHoverEvent *event)
{
    m_hoverPos = float(event->pos().x()) / width();
    update();
}

void BarChart::hoverMoveEvent(QHoverEvent *event)
{
    m_hoverPos = float(event->pos().x()) / width();
    update();
}

void BarChart::hoverLeaveEvent(QHoverEvent *event)
{
    m_hoverPos = -1.0f;
    update();
}

void BarChart::mousePressEvent(QMouseEvent *event)
{
    QCursor dragCursor(Qt::SizeHorCursor);
    setCursor(dragCursor);
}

void BarChart::mouseMoveEvent(QMouseEvent *event)
{
}

void BarChart::mouseReleaseEvent(QMouseEvent *event)
{
    unsetCursor();
}
