#include "barchart.h"

#include <algorithm>

#include "geometry.h"
#include "scale.h"

static const QColor OUTLINE_COLOR(0, 0, 0);
static const QColor BAR_COLOR(128, 128, 128);
static const float DEFAULT_OPACITY = 0.8f;

BarChart::BarChart(QQuickItem *parent)
    : QQuickItem(parent)
    , m_shouldUpdateBars(false)
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
    // setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

BarChart::~BarChart()
{
}

void BarChart::setValues(const arma::vec &values)
{
    m_values = values;
    m_shouldUpdateBars = true;
    emit valuesChanged(values);
}

QSGNode *BarChart::newBarNode() const
{
    // A bar node is:
    // opacityNode [outlineGeomNode barGeomNode]

    QSGGeometryNode *outlineGeomNode = new QSGGeometryNode;
    QSGGeometry *outlineGeometry =
        new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    outlineGeometry->setDrawingMode(GL_LINE_LOOP);
    outlineGeomNode->setGeometry(outlineGeometry);
    outlineGeomNode->setFlag(QSGNode::OwnsGeometry);
    QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
    material->setColor(OUTLINE_COLOR);
    outlineGeomNode->setMaterial(material);
    outlineGeomNode->setFlag(QSGNode::OwnsMaterial);

    QSGGeometryNode *barGeomNode = new QSGGeometryNode;
    QSGGeometry *barGeometry =
        new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    barGeometry->setDrawingMode(GL_POLYGON);
    barGeomNode->setGeometry(barGeometry);
    barGeomNode->setFlag(QSGNode::OwnsGeometry);
    material = new QSGFlatColorMaterial;
    material->setColor(BAR_COLOR);
    barGeomNode->setMaterial(material);
    barGeomNode->setFlag(QSGNode::OwnsMaterial);

    QSGOpacityNode *opacityNode = new QSGOpacityNode;
    opacityNode->setOpacity(DEFAULT_OPACITY);
    opacityNode->appendChildNode(barGeomNode);
    opacityNode->appendChildNode(outlineGeomNode);

    return opacityNode;
}

void BarChart::updateBarNodeGeom(QSGNode *barNode, float x, float barWidth, float barHeight)
{
    QSGGeometryNode *outlineGeomNode =
        static_cast<QSGGeometryNode *>(barNode->firstChild());
    QSGGeometryNode *barGeomNode =
        static_cast<QSGGeometryNode *>(barNode->firstChild()->nextSibling());

    float y = height() - barHeight;
    updateRectGeometry(outlineGeomNode->geometry(), x, y, barWidth, barHeight);
    updateRectGeometry(barGeomNode->geometry(), x, y, barWidth, barHeight);
}

void BarChart::updateBars(QSGNode *root)
{
    QSGNode *node = root->firstChild();
    float x = 0;
    float barWidth = width() / m_values.n_elem;
    LinearScale<float> heightScale(m_values.min(), m_values.max(), 0, height());

    for (auto it = m_values.cbegin(); it != m_values.cend(); it++) {
        updateBarNodeGeom(node, x, barWidth, heightScale((float) *it));
        x += barWidth;
        node = node->nextSibling();
    }
}

QSGNode *BarChart::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode ? oldNode : new QSGNode;
    if (m_shouldUpdateBars) {
        int numValues = (int) m_values.n_elem;
        // First, make sure we have the same number of values & bars
        while (numValues > root->childCount()) {
            QSGNode *barNode = newBarNode();
            root->appendChildNode(barNode);
        }
        while (numValues < root->childCount()) {
            // NOTE: as stated in docs, QSGNode's children are stored in a
            // linked list. Hence, this operation should be as fast as expected
            root->removeChildNode(root->firstChild());
        }

        // Then, update the geometry of bars to reflect the values
        updateBars(root);
        m_shouldUpdateBars = false;
    }

    return root;
}

void BarChart::hoverMoveEvent(QHoverEvent *event)
{
    // TODO
}

void BarChart::mousePressEvent(QMouseEvent *event)
{
    // TODO
}
