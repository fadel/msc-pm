#include "barchart.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>

#include "continuouscolorscale.h"
#include "geometry.h"

static const QColor OUTLINE_COLOR(0, 0, 0);
static const QColor BRUSH_COLOR(0, 0, 0);
static const QColor BAR_COLOR(128, 128, 128);
static const QColor PRESELECTION_COLOR(128, 128, 128, 96);
static const QColor SELECTION_VISIBLE_COLOR(128, 128, 128, 96);
static const QColor SELECTION_INVISIBLE_COLOR(0, 0, 0, 0);

static const float DEFAULT_OPACITY = 0.8f;

template<typename T>
static inline T clamp(T value, T min, T max)
{
    return std::min(std::max(min, value), max);
}

BarChart::BarChart(QQuickItem *parent)
    : QQuickItem(parent)
    , m_shouldUpdateBars(false)
    , m_shouldUpdatePreSelection(false)
    , m_dragStartPos(-1.0f)
    , m_dragLastPos(-1.0f)
    , m_shouldUpdateSelection(false)
    , m_brushedItem(-1)
    , m_colorScale(0)
    , m_scale(0.0f, 1.0f, 0.0f, 1.0f)
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
    //setAcceptedMouseButtons(Qt::LeftButton);
}

BarChart::~BarChart()
{
}

void BarChart::setValues(const arma::vec &values)
{
    m_values = values;

    if (m_selection.size() != m_values.n_elem) {
        m_selection.resize(m_values.n_elem);
        m_selection.assign(m_selection.size(), false);
    }

    m_originalIndices.resize(m_values.n_elem);
    m_currentIndices.resize(m_values.n_elem);
    setAcceptHoverEvents(m_values.n_elem > 0);
    if (m_values.n_elem > 0) {
        m_scale.setDomain(m_values.min(), m_values.max());

        // m_originalIndices[i] == the original (datum) index of item i
        std::iota(m_originalIndices.begin(), m_originalIndices.end(), 0);
        std::sort(m_originalIndices.begin(), m_originalIndices.end(),
            [this](int i, int j) { return m_values[i] > m_values[j]; });

        // m_currentIndices[i] == the displayed position of datum i
        int k = 0;
        for (auto i: m_originalIndices) {
            m_currentIndices[i] = k++;
        }
    }
    emit valuesChanged(values);

    m_shouldUpdateSelection = true;
    m_shouldUpdateBars = true;
    update();
}

void BarChart::updateValues(const arma::vec &values)
{
    if (m_values.n_elem != values.n_elem) {
        return;
    }

    m_values = values;
    m_shouldUpdateBars = true;
    update();
}

void BarChart::setColorScale(const ColorScale *scale)
{
    m_colorScale = scale;
    emit colorScaleChanged(m_colorScale);

    m_shouldUpdateBars = true;
    update();
}

void BarChart::setSelection(const std::vector<bool> &selection)
{
    m_selection = selection;
    emit selectionChanged(m_selection);

    // Our selection changed but we don't want to display it unless we have the
    // right number of values
    if (m_values.size() == m_selection.size()) {
        m_shouldUpdateSelection = true;
        update();
    }
}

void BarChart::brushItem(int item)
{
    if (item < 0) {
        m_brushedItem = item;
        emit itemBrushed(m_brushedItem, 0.0f);
    } else {
        // safe comparison: we just checked for negative values
        if (m_values.n_elem == 0 || item > m_values.n_elem - 1) {
            return;
        }

        m_brushedItem = m_currentIndices[item];
        item = m_originalIndices[m_brushedItem];
        emit itemBrushed(item, m_values[item]);
    }

    update();
}

QSGNode *BarChart::newSceneGraph() const
{
    // NOTE: scene graph structure is as follows:
    // root [
    //     barsNode [
    //         barNode ...
    //     ]
    //     selectedBarsNode [
    //         selectedBarNode ...
    //     ]
    //     preSelectionNode
    //     brushNode
    // ]
    QSGTransformNode *root = new QSGTransformNode;

    // The node that has all bars as children
    root->appendChildNode(new QSGNode);

    // The node that has all selected bars as children
    root->appendChildNode(new QSGNode);

    // The node for the preselection
    QSGSimpleRectNode *preSelectionNode = new QSGSimpleRectNode;
    preSelectionNode->setColor(PRESELECTION_COLOR);
    root->appendChildNode(preSelectionNode);

    // The node for drawing the brush
    QSGGeometryNode *brushGeomNode = new QSGGeometryNode;
    QSGGeometry *brushGeom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    brushGeom->setDrawingMode(GL_POLYGON);
    brushGeom->setVertexDataPattern(QSGGeometry::DynamicPattern);
    brushGeom->setLineWidth(1.0f);
    QSGFlatColorMaterial *brushMaterial = new QSGFlatColorMaterial;
    brushMaterial->setColor(BRUSH_COLOR);
    brushGeomNode->setGeometry(brushGeom);
    brushGeomNode->setMaterial(brushMaterial);
    brushGeomNode->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    root->appendChildNode(brushGeomNode);

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

QSGNode *BarChart::newSelectionBarNode() const
{
    QSGSimpleRectNode *node = new QSGSimpleRectNode;
    return node;
}

void BarChart::updateViewport(QSGNode *root) const
{
    QSGTransformNode *viewportNode = static_cast<QSGTransformNode *>(root);
    QMatrix4x4 viewport;
    viewport.scale(width(), height());
    viewportNode->setMatrix(viewport);
}

void BarChart::updateBarNodeGeom(QSGNode *barNode,
                                 float x,
                                 float barWidth,
                                 float barHeight) const
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

void BarChart::updateBarNodeColor(QSGNode *barNode, const QColor &color) const
{
    QSGGeometryNode *barGeomNode =
        static_cast<QSGGeometryNode *>(barNode->firstChild());

    QSGFlatColorMaterial *material =
        static_cast<QSGFlatColorMaterial *>(barGeomNode->material());
    material->setColor(color);
    barGeomNode->markDirty(QSGNode::DirtyMaterial);
}

void BarChart::updateBars(QSGNode *node) const
{
    int numValues = (int) m_values.n_elem;
    float barWidth = 1.0f / numValues;

    // First, make sure we have the same number of values & bars
    while (numValues > node->childCount()) {
        QSGNode *barNode = newBarNode();
        node->prependChildNode(barNode);
    }
    while (numValues < node->childCount()) {
        // NOTE: as stated in docs, QSGNode's children are stored in a
        // linked list. Hence, this operation should be as fast as expected
        node->removeChildNode(node->firstChild());
    }

    // Then, update each bar to reflect the values
    node = node->firstChild();
    float x = 0;
    for (auto it = m_originalIndices.cbegin(); it != m_originalIndices.cend(); it++) {
        updateBarNodeGeom(node, x, barWidth, m_scale(m_values[*it]));
        updateBarNodeColor(node, m_colorScale->color(m_values[*it]));
        x += barWidth;
        node = node->nextSibling();
    }
}

void BarChart::updateSelectionBar(QSGNode *node,
                                  float x,
                                  float barWidth,
                                  const QColor &color) const
{
    QSGSimpleRectNode *barNode = static_cast<QSGSimpleRectNode *>(node);
    barNode->setRect(x, 0, barWidth, 1.0f);
    barNode->setColor(color);
}

void BarChart::updateSelectionBars(QSGNode *node) const
{
    int numValues = (int) m_values.n_elem;
    float barWidth = 1.0f / numValues;

    // First, make sure we have the same number of values & bars
    while (numValues > node->childCount()) {
        QSGNode *barNode = newSelectionBarNode();
        node->prependChildNode(barNode);
    }
    while (numValues < node->childCount()) {
        // NOTE: as stated in docs, QSGNode's children are stored in a
        // linked list. Hence, this operation should be as fast as expected
        node->removeChildNode(node->firstChild());
    }

    // Update each bar, displaying it (using a visible color) only if it is a
    // selected item
    float x = 0;
    node = node->firstChild();
    for (auto it = m_originalIndices.cbegin(); it != m_originalIndices.cend(); it++) {
        updateSelectionBar(node, x, barWidth,
                           m_selection[*it] ? SELECTION_VISIBLE_COLOR
                                            : SELECTION_INVISIBLE_COLOR);
        x += barWidth;
        node = node->nextSibling();
    }
}

void BarChart::updatePreSelection(QSGNode *node) const
{
    QSGSimpleRectNode *preSelectionNode = static_cast<QSGSimpleRectNode *>(node);
    preSelectionNode->setRect(std::min(m_dragStartPos, m_dragLastPos), 0.0f,
                              fabs(m_dragLastPos - m_dragStartPos), 1.0f);
}

void BarChart::updateBrush(QSGNode *node) const
{
    float barWidth = 1.0f / m_values.n_elem;
    QSGGeometryNode *brushGeomNode = static_cast<QSGGeometryNode *>(node);
    updateRectGeometry(brushGeomNode->geometry(),
                       barWidth * m_brushedItem,
                       0.0f,
                       barWidth,
                       1.0f);
    brushGeomNode->markDirty(QSGNode::DirtyGeometry);
}

QSGNode *BarChart::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode ? oldNode : newSceneGraph();
    updateViewport(root);

    QSGNode *node = root->firstChild();
    if (m_shouldUpdateBars) {
        updateBars(node);
        m_shouldUpdateBars = false;
    }
    node = node->nextSibling();

    if (m_shouldUpdateSelection) {
        updateSelectionBars(node);
        m_shouldUpdateSelection = false;
    }
    node = node->nextSibling();

    if (m_shouldUpdatePreSelection) {
        updatePreSelection(node);
        m_shouldUpdatePreSelection = false;
    }
    node = node->nextSibling();

    updateBrush(node);
    node = node->nextSibling();

    return root;
}

int BarChart::itemAt(float x, bool includeSelectorWidth) const
{
    int numValues = m_values.n_elem;
    float barWidth = 1.0f / numValues;
    if (includeSelectorWidth) {
        x += 1.0f / width();
    }

    return clamp(int(x / barWidth), 0, numValues - 1);
}

void BarChart::interactiveSelection(float start, float end)
{
    if (start > end) {
        std::swap(start, end);
    }

    m_selection.assign(m_selection.size(), false);
    if (start < 0.0f || end < 0.0f) {
        return;
    }

    // Bars are located in ranges:
    // [0..barWidth] + barIndex / barWidth
    int firstIndex = itemAt(start);
    int lastIndex  = itemAt(end, true);
    for (int i = firstIndex; i <= lastIndex; i++) {
        m_selection[m_originalIndices[i]] = true;
    }

    emit selectionInteractivelyChanged(m_selection);
}

void BarChart::hoverEnterEvent(QHoverEvent *event)
{
    m_brushedItem = itemAt(float(event->pos().x()) / width());
    emit itemInteractivelyBrushed(m_originalIndices[m_brushedItem]);

    update();
}

void BarChart::hoverMoveEvent(QHoverEvent *event)
{
    m_brushedItem  = itemAt(float(event->pos().x()) / width());
    emit itemInteractivelyBrushed(m_originalIndices[m_brushedItem]);

    update();
}

void BarChart::hoverLeaveEvent(QHoverEvent *event)
{
    m_brushedItem = -1;
    emit itemInteractivelyBrushed(m_brushedItem);

    update();
}

void BarChart::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        m_dragStartPos = -1.0f;
        m_dragLastPos  = -1.0f;
        m_selection.assign(m_selection.size(), false);
        emit selectionInteractivelyChanged(m_selection);
    } else {
        QCursor dragCursor(Qt::SizeHorCursor);
        setCursor(dragCursor);

        float pos = float(event->pos().x()) / width();
        m_dragStartPos = pos;
        m_dragLastPos  = pos;
    }

    m_shouldUpdatePreSelection = true;
    update();
}

void BarChart::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragStartPos < 0.0f) {
        return;
    }

    float pos = float(event->pos().x()) / width();
    m_dragLastPos = clamp(pos, 0.0f, 1.0f);

    m_shouldUpdatePreSelection = true;
    update();
}

void BarChart::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragStartPos < 0.0f) {
        return;
    }

    unsetCursor();

    float pos = float(event->pos().x()) / width();
    m_dragLastPos = clamp(pos, 0.0f, 1.0f);

    if (m_values.n_elem > 0) {
        interactiveSelection(m_dragStartPos, m_dragLastPos);
    }

    m_dragStartPos = -1.0f;
    m_dragLastPos  = -1.0f;
    m_shouldUpdatePreSelection = true;
    update();
}
