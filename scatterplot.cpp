#include "scatterplot.h"

#include <algorithm>
#include <cmath>

#include <QSGGeometryNode>
#include <QSGSimpleRectNode>

#include "continuouscolorscale.h"
#include "geometry.h"

// Glyphs settings
static const float DEFAULT_GLYPH_SIZE = 8.0f;
static const qreal GLYPH_OPACITY = 1.0;
static const qreal GLYPH_OPACITY_SELECTED = 1.0;

static const float GLYPH_OUTLINE_WIDTH = 2.0f;
static const QColor GLYPH_OUTLINE_COLOR(0, 0, 0);
static const QColor GLYPH_OUTLINE_COLOR_SELECTED(20, 255, 225);

// Cosshair settings
static const float CROSSHAIR_LENGTH = 8.0f;
static const float CROSSHAIR_THICKNESS1 = 1.0f;
static const float CROSSHAIR_THICKNESS2 = 0.5f;
static const QColor CROSSHAIR_COLOR1(255, 255, 255);
static const QColor CROSSHAIR_COLOR2(0, 0, 0);

// Selection settings
static const QColor SELECTION_COLOR(128, 128, 128, 96);

class QuadTree
{
public:
    QuadTree(const QRectF &bounds);
    ~QuadTree();
    bool insert(float x, float y, int value);
    int query(float x, float y) const;

    bool subdivide();

    QRectF m_bounds;
    float m_x, m_y;
    int m_value;
    QuadTree *m_nw, *m_ne, *m_sw, *m_se;
};

QuadTree::QuadTree(const QRectF &bounds)
    : m_bounds(bounds)
    , m_value(-1)
    , m_nw(0), m_ne(0), m_sw(0), m_se(0)
{
}

QuadTree::~QuadTree()
{
    if (m_nw) {
        delete m_nw;
        delete m_ne;
        delete m_sw;
        delete m_se;
    }
}

bool QuadTree::subdivide()
{
    float halfWidth = m_bounds.width() / 2;
    float halfHeight = m_bounds.height() / 2;

    m_nw = new QuadTree(QRectF(m_bounds.x(),
                               m_bounds.y(),
                               halfWidth,
                               halfHeight));
    m_ne = new QuadTree(QRectF(m_bounds.x() + halfWidth,
                               m_bounds.y(),
                               halfWidth,
                               halfHeight));
    m_sw = new QuadTree(QRectF(m_bounds.x(),
                               m_bounds.y() + halfHeight,
                               halfWidth,
                               halfHeight));
    m_se = new QuadTree(QRectF(m_bounds.x() + halfWidth,
                               m_bounds.y() + halfHeight,
                               halfWidth,
                               halfHeight));

    int value = m_value;
    m_value = -1;
    return m_nw->insert(m_x, m_y, value)
        || m_ne->insert(m_x, m_y, value)
        || m_sw->insert(m_x, m_y, value)
        || m_se->insert(m_x, m_y, value);
}

bool QuadTree::insert(float x, float y, int value)
{
    if (!m_bounds.contains(x, y)) {
        return false;
    }

    if (m_nw) {
        return m_nw->insert(x, y, value)
            || m_ne->insert(x, y, value)
            || m_sw->insert(x, y, value)
            || m_se->insert(x, y, value);
    }

    if (m_value >= 0) {
        subdivide();
        return insert(x, y, value);
    }

    m_x = x;
    m_y = y;
    m_value = value;
    return true;
}

int QuadTree::query(float x, float y) const
{
    if (!m_bounds.contains(x, y)) {
        return -1;
    }

    if (m_nw) {
        int q;

        q = m_nw->query(x, y);
        if (q >= 0) return q;
        q = m_ne->query(x, y);
        if (q >= 0) return q;
        q = m_sw->query(x, y);
        if (q >= 0) return q;
        q = m_se->query(x, y);
        if (q >= 0) return q;
    }

    return m_value;
}

Scatterplot::Scatterplot(QQuickItem *parent)
    : QQuickItem(parent)
    , m_glyphSize(DEFAULT_GLYPH_SIZE)
    , m_colorScale(ContinuousColorScale::builtin(ContinuousColorScale::HEATED_OBJECTS))
    , m_autoScale(true)
    , m_sx(0, 1, 0, 1)
    , m_sy(0, 1, 0, 1)
    , m_brushedItem(-1)
    , m_currentInteractionState(INTERACTION_NONE)
    , m_shouldUpdateGeometry(false)
    , m_shouldUpdateMaterials(false)
    , m_quadtree(0)
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
}

void Scatterplot::setColorScale(const ColorScale &colorScale)
{
    m_colorScale = colorScale;
    if (m_colorData.n_elem > 0) {
        m_colorScale.setExtents(m_colorData.min(), m_colorData.max());
        m_shouldUpdateMaterials = true;
        update();
    }
}

arma::mat Scatterplot::XY() const
{
    return m_xy;
}

bool Scatterplot::saveToFile(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return false;
    }

    return m_xy.save(url.path().toStdString(), arma::raw_ascii);
}

void Scatterplot::setXY(const arma::mat &xy, bool updateView)
{
    if (xy.n_cols != 2) {
        return;
    }

    m_xy = xy;
    emit xyChanged(m_xy);

    if (m_autoScale) {
        autoScale();
    }

    if (m_selection.size() != m_xy.n_rows) {
        m_selection.resize(m_xy.n_rows);
        m_selection.assign(m_selection.size(), false);
    }

    if (m_opacityData.n_elem != m_xy.n_rows) {
        arma::vec opacityData(xy.n_rows);
        opacityData.fill(GLYPH_OPACITY);
        setOpacityData(opacityData, false);
    }

    m_shouldUpdateGeometry = true;
    if (updateView) {
        update();
    }
}

void Scatterplot::setXY(const arma::mat &xy)
{
    setXY(xy, true);
}

void Scatterplot::setColorData(const arma::vec &colorData, bool updateView)
{
    if (m_xy.n_rows > 0 && colorData.n_elem != m_xy.n_rows) {
        return;
    }

    m_colorData = colorData;
    emit colorDataChanged(m_colorData);

    m_colorScale.setExtents(m_colorData.min(), m_colorData.max());
    m_shouldUpdateMaterials = true;
    if (updateView) {
        update();
    }
}

void Scatterplot::setColorData(const arma::vec &colorData)
{
    setColorData(colorData, true);
}

void Scatterplot::setOpacityData(const arma::vec &opacityData, bool updateView)
{
    if (m_xy.n_rows > 0 && opacityData.n_elem != m_xy.n_rows) {
        return;
    }

    m_opacityData = opacityData;
    emit opacityDataChanged(m_opacityData);

    if (updateView) {
        update();
    }
}

void Scatterplot::setOpacityData(const arma::vec &opacityData)
{
    setOpacityData(opacityData, true);
}

void Scatterplot::setScale(const LinearScale<float> &sx, const LinearScale<float> &sy, bool updateView)
{
    m_sx = sx;
    m_sy = sy;
    emit scaleChanged(m_sx, m_sy);

    updateQuadTree();

    m_shouldUpdateGeometry = true;
    if (updateView) {
        update();
    }
}

void Scatterplot::setScale(const LinearScale<float> &sx, const LinearScale<float> &sy)
{
    setScale(sx, sy, true);
}

void Scatterplot::setAutoScale(bool autoScale)
{
    m_autoScale = autoScale;
    if (autoScale) {
        this->autoScale();
    }
}

void Scatterplot::autoScale()
{
    m_sx.setDomain(m_xy.col(0).min(), m_xy.col(0).max());
    m_sy.setDomain(m_xy.col(1).min(), m_xy.col(1).max());

    emit scaleChanged(m_sx, m_sy);
}

void Scatterplot::setGlyphSize(float glyphSize, bool updateView)
{
    m_glyphSize = glyphSize;
    emit glyphSizeChanged(m_glyphSize);

    m_shouldUpdateGeometry = true;
    if (updateView) {
        update();
    }
}

void Scatterplot::setGlyphSize(float glyphSize)
{
    setGlyphSize(glyphSize, true);
}

QSGNode *Scatterplot::newSceneGraph()
{
    // NOTE:
    // The hierarchy in the scene graph is as follows:
    // root [[splatNode] [glyphsRoot [glyph [...]]] [selectionNode]]
    QSGNode *root = new QSGNode;
    QSGNode *glyphTreeRoot = newGlyphTree();
    if (glyphTreeRoot) {
        root->appendChildNode(glyphTreeRoot);
    }

    QSGSimpleRectNode *selectionRectNode = new QSGSimpleRectNode;
    selectionRectNode->setColor(SELECTION_COLOR);
    root->appendChildNode(selectionRectNode);

    QSGTransformNode *brushNode = new QSGTransformNode;

    QSGGeometryNode *whiteCrossHairNode = new QSGGeometryNode;
    QSGGeometry *whiteCrossHairGeom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 12);
    whiteCrossHairGeom->setDrawingMode(GL_POLYGON);
    whiteCrossHairGeom->setVertexDataPattern(QSGGeometry::DynamicPattern);
    updateCrossHairGeometry(whiteCrossHairGeom, 0, 0, CROSSHAIR_THICKNESS1, CROSSHAIR_LENGTH);
    QSGFlatColorMaterial *whiteCrossHairMaterial = new QSGFlatColorMaterial;
    whiteCrossHairMaterial->setColor(CROSSHAIR_COLOR1);
    whiteCrossHairNode->setGeometry(whiteCrossHairGeom);
    whiteCrossHairNode->setMaterial(whiteCrossHairMaterial);
    whiteCrossHairNode->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    brushNode->appendChildNode(whiteCrossHairNode);

    QSGGeometryNode *blackCrossHairNode = new QSGGeometryNode;
    QSGGeometry *blackCrossHairGeom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 12);
    blackCrossHairGeom->setDrawingMode(GL_POLYGON);
    blackCrossHairGeom->setVertexDataPattern(QSGGeometry::DynamicPattern);
    updateCrossHairGeometry(blackCrossHairGeom, 0, 0, CROSSHAIR_THICKNESS2, CROSSHAIR_LENGTH);
    QSGFlatColorMaterial *blackCrossHairMaterial = new QSGFlatColorMaterial;
    blackCrossHairMaterial->setColor(CROSSHAIR_COLOR2);
    blackCrossHairNode->setGeometry(blackCrossHairGeom);
    blackCrossHairNode->setMaterial(blackCrossHairMaterial);
    blackCrossHairNode->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    brushNode->appendChildNode(blackCrossHairNode);

    root->appendChildNode(brushNode);

    return root;
}

QSGNode *Scatterplot::newGlyphTree()
{
    // NOTE:
    // The glyph graph is structured as:
    // root [opacityNode [outlineNode fillNode] ...]
    if (m_xy.n_rows < 1) {
        return 0;
    }

    QSGNode *node = new QSGNode;
    int vertexCount = calculateCircleVertexCount(m_glyphSize);

    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        QSGGeometry *glyphOutlineGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        glyphOutlineGeometry->setDrawingMode(GL_POLYGON);
        glyphOutlineGeometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        QSGGeometryNode *glyphOutlineNode = new QSGGeometryNode;
        glyphOutlineNode->setGeometry(glyphOutlineGeometry);
        glyphOutlineNode->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(GLYPH_OUTLINE_COLOR);
        glyphOutlineNode->setMaterial(material);
        glyphOutlineNode->setFlag(QSGNode::OwnsMaterial);

        QSGGeometry *glyphGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        glyphGeometry->setDrawingMode(GL_POLYGON);
        glyphGeometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        QSGGeometryNode *glyphNode = new QSGGeometryNode;
        glyphNode->setGeometry(glyphGeometry);
        glyphNode->setFlag(QSGNode::OwnsGeometry);

        material = new QSGFlatColorMaterial;
        material->setColor(QColor());
        glyphNode->setMaterial(material);
        glyphNode->setFlag(QSGNode::OwnsMaterial);

        // Place the glyph geometry node under an opacity node
        QSGOpacityNode *glyphOpacityNode = new QSGOpacityNode;
        glyphOpacityNode->appendChildNode(glyphOutlineNode);
        glyphOpacityNode->appendChildNode(glyphNode);
        node->appendChildNode(glyphOpacityNode);
    }

    return node;
}

QSGNode *Scatterplot::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode ? oldNode : newSceneGraph();

    if (m_xy.n_rows < 1) {
        return root;
    }

    // This keeps track of where we are in the scene when updating
    QSGNode *node = root->firstChild();

    updateGlyphs(node);
    node = node->nextSibling();

    if (m_shouldUpdateGeometry) {
        m_shouldUpdateGeometry = false;
    }
    if (m_shouldUpdateMaterials) {
        m_shouldUpdateMaterials = false;
    }

    // Selection
    QSGSimpleRectNode *selectionNode = static_cast<QSGSimpleRectNode *>(node);
    if (m_currentInteractionState == INTERACTION_SELECTING) {
        selectionNode->setRect(QRectF(m_dragOriginPos, m_dragCurrentPos));
        selectionNode->markDirty(QSGNode::DirtyGeometry);
    } else {
        // Hide selection rect
        selectionNode->setRect(QRectF(-1, -1, 0, 0));
    }
    node = node->nextSibling();

    // Brushing
    updateBrush(node);
    node = node->nextSibling();

    return root;
}

void Scatterplot::updateGlyphs(QSGNode *glyphsNode)
{
    qreal x, y, tx, ty, moveTranslationF;

    if (!m_shouldUpdateGeometry && !m_shouldUpdateMaterials) {
        return;
    }

    if (m_currentInteractionState == INTERACTION_MOVING) {
        tx = m_dragCurrentPos.x() - m_dragOriginPos.x();
        ty = m_dragCurrentPos.y() - m_dragOriginPos.y();
    } else {
        tx = ty = 0;
    }

    m_sx.setRange(PADDING, width() - PADDING);
    m_sy.setRange(height() - PADDING, PADDING);

    QSGNode *node = glyphsNode->firstChild();
    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        const arma::rowvec &row = m_xy.row(i);
        bool isSelected = m_selection[i];

        QSGOpacityNode *glyphOpacityNode = static_cast<QSGOpacityNode *>(node);
        glyphOpacityNode->setOpacity(m_opacityData[i]);

        QSGGeometryNode *glyphOutlineNode = static_cast<QSGGeometryNode *>(node->firstChild());
        QSGGeometryNode *glyphNode = static_cast<QSGGeometryNode *>(node->firstChild()->nextSibling());
        if (m_shouldUpdateGeometry) {
            moveTranslationF = isSelected ? 1.0 : 0.0;
            x = m_sx(row[0]) + tx * moveTranslationF;
            y = m_sy(row[1]) + ty * moveTranslationF;

            QSGGeometry *geometry = glyphOutlineNode->geometry();
            updateCircleGeometry(geometry, m_glyphSize, x, y);
            glyphOutlineNode->markDirty(QSGNode::DirtyGeometry);

            geometry = glyphNode->geometry();
            updateCircleGeometry(geometry, m_glyphSize - 2*GLYPH_OUTLINE_WIDTH, x, y);
            glyphNode->markDirty(QSGNode::DirtyGeometry);
        }
        if (m_shouldUpdateMaterials) {
            QSGFlatColorMaterial *material = static_cast<QSGFlatColorMaterial *>(glyphOutlineNode->material());
            material->setColor(isSelected ? GLYPH_OUTLINE_COLOR_SELECTED : GLYPH_OUTLINE_COLOR);
            glyphOutlineNode->markDirty(QSGNode::DirtyMaterial);

            material = static_cast<QSGFlatColorMaterial *>(glyphNode->material());
            material->setColor(m_colorScale.color(m_colorData[i]));
            glyphNode->markDirty(QSGNode::DirtyMaterial);
        }

        node = node->nextSibling();
    }
}

void Scatterplot::updateBrush(QSGNode *node)
{
    QMatrix4x4 transform;
    if (m_brushedItem < 0) {
        transform.translate(-width(), -height());
    } else {
        const arma::rowvec &row = m_xy.row(m_brushedItem);
        transform.translate(m_sx(row[0]), m_sy(row[1]));
    }

    QSGTransformNode *brushNode = static_cast<QSGTransformNode *>(node);
    brushNode->setMatrix(transform);
}

void Scatterplot::mousePressEvent(QMouseEvent *event)
{
    switch (m_currentInteractionState) {
    case INTERACTION_NONE:
    case INTERACTION_SELECTED:
        if (event->modifiers() == Qt::ShiftModifier) {
            m_currentInteractionState = INTERACTION_BEGIN_MOVING;
        } else {
            m_currentInteractionState = INTERACTION_SELECTING;
        }
        m_dragOriginPos = event->localPos();
        m_dragCurrentPos = m_dragOriginPos;
        break;
    case INTERACTION_SELECTING:
    case INTERACTION_BEGIN_MOVING:
    case INTERACTION_MOVING:
        event->ignore();
        break;
    }
}

void Scatterplot::mouseMoveEvent(QMouseEvent *event)
{
    switch (m_currentInteractionState) {
    case INTERACTION_SELECTING:
        m_dragCurrentPos = event->localPos();
        update();
        break;
    case INTERACTION_BEGIN_MOVING:
        m_currentInteractionState = INTERACTION_MOVING;
    case INTERACTION_MOVING:
        m_dragCurrentPos = event->localPos();
        m_shouldUpdateGeometry = true;
        update();
        break;
    case INTERACTION_NONE:
    case INTERACTION_SELECTED:
        event->ignore();
        return;
    }
}

void Scatterplot::mouseReleaseEvent(QMouseEvent *event)
{
    switch (m_currentInteractionState) {
    case INTERACTION_SELECTING:
        {
            bool mergeSelection = (event->modifiers() == Qt::ControlModifier);
            bool anySelected = interactiveSelection(mergeSelection);
            m_currentInteractionState = anySelected ? INTERACTION_SELECTED
                                                    : INTERACTION_NONE;
            m_shouldUpdateMaterials = true;
            update();
        }
        break;
    case INTERACTION_BEGIN_MOVING:
        m_currentInteractionState = INTERACTION_SELECTED;
        break;
    case INTERACTION_MOVING:
        m_currentInteractionState = INTERACTION_SELECTED;
        applyManipulation();
        m_shouldUpdateGeometry = true;
        update();
        m_dragOriginPos = m_dragCurrentPos;
        break;
    case INTERACTION_NONE:
    case INTERACTION_SELECTED:
        break; // should not be reached
    }
}

void Scatterplot::hoverEnterEvent(QHoverEvent *event)
{
    QPointF pos = event->posF();
    m_brushedItem = m_quadtree->query(pos.x(), pos.y());
    emit itemInteractivelyBrushed(m_brushedItem);

    update();
}

void Scatterplot::hoverMoveEvent(QHoverEvent *event)
{
    QPointF pos = event->posF();
    m_brushedItem = m_quadtree->query(pos.x(), pos.y());
    emit itemInteractivelyBrushed(m_brushedItem);

    update();
}

void Scatterplot::hoverLeaveEvent(QHoverEvent *event)
{
    m_brushedItem = -1;
    emit itemInteractivelyBrushed(m_brushedItem);

    update();
}

bool Scatterplot::interactiveSelection(bool mergeSelection)
{
    if (!mergeSelection) {
        m_selection.assign(m_selection.size(), false);
    }

    m_sx.inverse();
    m_sy.inverse();
    QRectF selectionRect(QPointF(m_sx(m_dragOriginPos.x()),
                                 m_sy(m_dragOriginPos.y())),
                         QPointF(m_sx(m_dragCurrentPos.x()),
                                 m_sy(m_dragCurrentPos.y())));
    m_sy.inverse();
    m_sx.inverse();

    bool anySelected = false;
    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        const arma::rowvec &row = m_xy.row(i);

        if (selectionRect.contains(row[0], row[1])) {
            m_selection[i] = true;
            anySelected = true;
        }
    }

    emit selectionInteractivelyChanged(m_selection);
    return anySelected;
}

void Scatterplot::setSelection(const std::vector<bool> &selection)
{
    if (m_selection.size() != selection.size()) {
        return;
    }

    m_selection = selection;
    emit selectionChanged(m_selection);

    m_shouldUpdateMaterials = true;
    update();
}

void Scatterplot::brushItem(int item)
{
    m_brushedItem = item;
    update();
}

void Scatterplot::applyManipulation()
{
    m_sx.inverse();
    m_sy.inverse();
    LinearScale<float> rx = m_sx;
    LinearScale<float> ry = m_sy;
    m_sy.inverse();
    m_sx.inverse();

    float tx = m_dragCurrentPos.x() - m_dragOriginPos.x();
    float ty = m_dragCurrentPos.y() - m_dragOriginPos.y();

    for (std::vector<bool>::size_type i = 0; i < m_selection.size(); i++) {
        if (m_selection[i]) {
            arma::rowvec row = m_xy.row(i);
            row[0] = rx(m_sx(row[0]) + tx);
            row[1] = ry(m_sy(row[1]) + ty);
            m_xy.row(i) = row;
        }
    }

    updateQuadTree();

    emit xyInteractivelyChanged(m_xy);
}

void Scatterplot::updateQuadTree()
{
    m_sx.setRange(PADDING, width() - PADDING);
    m_sy.setRange(height() - PADDING, PADDING);

    if (m_quadtree) {
        delete m_quadtree;
    }

    m_quadtree = new QuadTree(QRectF(x(), y(), width(), height()));
    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        const arma::rowvec &row = m_xy.row(i);
        m_quadtree->insert(m_sx(row[0]), m_sy(row[1]), (int) i);
    }
}
