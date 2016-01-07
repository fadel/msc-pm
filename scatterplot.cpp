#include "scatterplot.h"

#include "voronoisplat.h"
#include "geometry.h"
#include <cmath>

static const qreal GLYPH_OPACITY = 0.4;
static const qreal GLYPH_OPACITY_SELECTED = 1.0;

static const QColor OUTLINE_COLOR(0, 0, 0);
static const QColor SELECTION_COLOR(128, 128, 128, 96);

static const int GLYPH_SIZE = 4.f;
static const float PADDING = 10.f;

Scatterplot::Scatterplot(QQuickItem *parent)
    : QQuickItem(parent)
    , m_sx(0, 1, 0, 1)
    , m_sy(0, 1, 0, 1)
    , m_currentInteractionState(INTERACTION_NONE)
    , m_shouldUpdateGeometry(false)
    , m_shouldUpdateMaterials(false)
    , m_animationEasing(QEasingCurve::InOutQuart)
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
}

void Scatterplot::setColorScale(ColorScale *colorScale)
{
    if (!colorScale) {
        return;
    }

    m_colorScale = colorScale;
    if (m_colorData.n_elem > 0) {
        updateMaterials();
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

void Scatterplot::setXY(const arma::mat &xy)
{
    if (xy.n_cols != 2) {
        return;
    }

    if (m_xy.n_elem != xy.n_elem) {
        m_oldXY = xy;
        m_selectedGlyphs.clear();
    } else {
        m_oldXY = m_xy;
    }

    m_xy = xy;

    float min = std::min(m_xy.col(0).min(), m_oldXY.col(0).min());
    float max = std::max(m_xy.col(0).max(), m_oldXY.col(0).max());
    m_sx.setDomain(min, max);
    min = std::min(m_xy.col(1).min(), m_oldXY.col(1).min());
    max = std::max(m_xy.col(1).max(), m_oldXY.col(1).max());
    m_sy.setDomain(min, max);

    updateGeometry();

    emit xyChanged(m_xy);

    startAnimation();
}

void Scatterplot::setColorData(const arma::vec &colorData)
{
    if (colorData.n_elem != m_xy.n_rows) {
        return;
    }

    m_colorData = colorData;
    emit colorDataChanged(m_colorData);

    updateMaterials();
}

void Scatterplot::updateGeometry()
{
    m_shouldUpdateGeometry = true;
    update();
}

void Scatterplot::updateMaterials()
{
    m_shouldUpdateMaterials = true;
    update();
}

QSGNode *Scatterplot::newSplatNode()
{
    if (m_xy.n_rows < 1) {
        return 0;
    }

    QSGSimpleTextureNode *node = new QSGSimpleTextureNode;
    VoronoiSplatTexture *tex = new VoronoiSplatTexture(QSize(width(), height()));

    node->setTexture(tex);
    node->setOwnsTexture(true);
    node->setRect(x(), y(), width(), height());
    node->setSourceRect(0, 0, width(), height());

    tex->setSites(m_xy);
    tex->setValues(m_colorData);
    tex->setColormap(m_colorScale);
    tex->updateTexture();

    window()->resetOpenGLState();

    return node;
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
    int vertexCount = calculateCircleVertexCount(GLYPH_SIZE / 2);

    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        QSGGeometry *glyphOutlineGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        glyphOutlineGeometry->setDrawingMode(GL_LINE_LOOP);
        updateCircleGeometry(glyphOutlineGeometry, GLYPH_SIZE / 2, 0, 0);
        QSGGeometryNode *glyphOutlineNode = new QSGGeometryNode;
        glyphOutlineNode->setGeometry(glyphOutlineGeometry);
        glyphOutlineNode->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(OUTLINE_COLOR);
        glyphOutlineNode->setMaterial(material);
        glyphOutlineNode->setFlag(QSGNode::OwnsMaterial);

        QSGGeometry *glyphGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        glyphGeometry->setDrawingMode(GL_POLYGON);
        updateCircleGeometry(glyphGeometry, GLYPH_SIZE / 2 - 0.5, 0, 0);
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

QSGNode *Scatterplot::newSceneGraph()
{
    // NOTE:
    // The hierarchy in the scene graph is as follows:
    // root [[splatNode] [glyphsRoot [glyph [...]]] [selectionNode]]
    QSGNode *root = new QSGNode;
    QSGNode *splatNode = newSplatNode();
    if (splatNode) {
        root->appendChildNode(splatNode);
    }
    QSGNode *glyphTreeRoot = newGlyphTree();
    if (glyphTreeRoot) {
        root->appendChildNode(glyphTreeRoot);
    }

    QSGSimpleRectNode *selectionRectNode = new QSGSimpleRectNode;
    selectionRectNode->setColor(SELECTION_COLOR);
    root->appendChildNode(selectionRectNode);

    return root;
}

QSGNode *Scatterplot::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode ? oldNode : newSceneGraph();

    if (m_xy.n_rows < 1) {
        return root;
    }

    QSGNode *splatNode = root->firstChild();
    updateSplat(splatNode);

    QSGNode *glyphsRootNode = root->firstChild()->nextSibling();
    updateGlyphs(glyphsRootNode->firstChild());

    // Change update hints to false; the splat and glyphs were just updated
    if (m_shouldUpdateGeometry) {
        m_shouldUpdateGeometry = false;
    }
    if (m_shouldUpdateMaterials) {
        m_shouldUpdateMaterials = false;
    }

    // Selection
    QSGSimpleRectNode *selectionRectNode =
        static_cast<QSGSimpleRectNode *>(root->firstChild()->nextSibling()->nextSibling());
    if (m_currentInteractionState == INTERACTION_SELECTING) {
        selectionRectNode->setRect(QRectF(m_dragOriginPos, m_dragCurrentPos));
        selectionRectNode->markDirty(QSGNode::DirtyGeometry);
    } else {
        // Hide selection rect
        selectionRectNode->setRect(QRectF(-1, -1, 0, 0));
    }

    animationTick();
    return root;
}

void Scatterplot::updateSplat(QSGNode *node)
{
    QSGSimpleTextureNode *texNode = static_cast<QSGSimpleTextureNode *>(node);
    VoronoiSplatTexture *tex =
        static_cast<VoronoiSplatTexture *>(texNode->texture());

    if (m_shouldUpdateGeometry) {
        tex->setSites(m_xy);
    }

    if (m_shouldUpdateMaterials) {
        tex->setValues(m_colorData);
        tex->setColormap(m_colorScale);
    }

    bool updated = tex->updateTexture();
    if (updated) {
        window()->resetOpenGLState();
    }
}

void Scatterplot::updateGlyphs(QSGNode *node)
{

    qreal x, y, tx, ty, moveTranslationF;

    if (m_currentInteractionState == INTERACTION_MOVING) {
        tx = m_dragCurrentPos.x() - m_dragOriginPos.x();
        ty = m_dragCurrentPos.y() - m_dragOriginPos.y();
    } else {
        tx = ty = 0;
    }

    m_sx.setRange(PADDING, width() - PADDING);
    m_sy.setRange(height() - PADDING, PADDING);

    float t = m_animationEasing.valueForProgress(m_t);
    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        const arma::rowvec &oldRow = m_oldXY.row(i);
        const arma::rowvec &row = m_xy.row(i);
        bool isSelected = m_selectedGlyphs.contains(i);

        QSGOpacityNode *glyphOpacityNode = static_cast<QSGOpacityNode *>(node);
        glyphOpacityNode->setOpacity(isSelected ? GLYPH_OPACITY_SELECTED : GLYPH_OPACITY);

        QSGGeometryNode *glyphOutlineNode = static_cast<QSGGeometryNode *>(node->firstChild());
        QSGGeometryNode *glyphNode = static_cast<QSGGeometryNode *>(node->firstChild()->nextSibling());
        if (m_shouldUpdateGeometry) {
            moveTranslationF = isSelected ? 1.0 : 0.0;
            x = m_sx(t*row[0] + (1 - t)*oldRow[0]) + tx * moveTranslationF;
            y = m_sy(t*row[1] + (1 - t)*oldRow[1]) + ty * moveTranslationF;

            QSGGeometry *geometry = glyphOutlineNode->geometry();
            updateCircleGeometry(geometry, GLYPH_SIZE / 2, x, y);
            glyphOutlineNode->markDirty(QSGNode::DirtyGeometry);

            geometry = glyphNode->geometry();
            updateCircleGeometry(geometry, GLYPH_SIZE / 2 - 0.5, x, y);
            glyphNode->markDirty(QSGNode::DirtyGeometry);
        }
        if (m_shouldUpdateMaterials) {
            QSGFlatColorMaterial *material = static_cast<QSGFlatColorMaterial *>(glyphNode->material());
            material->setColor(m_colorScale->color(m_colorData[i]));
            glyphNode->setMaterial(material);
            glyphNode->markDirty(QSGNode::DirtyMaterial);
        }

        node = node->nextSibling();
    }
}

void Scatterplot::resetAnimation()
{
    m_t = 0;
}

void Scatterplot::startAnimation()
{
    if (m_t < 1.0f) {
        m_t = 1.0f;
        return;
    }

    resetAnimation();
    update();
}

void Scatterplot::animationTick()
{
    if (m_t < 1.0f) {
        m_t += 0.1f;
        updateGeometry();
    }
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
    case INTERACTION_NONE:
        // event->localPos()
        break;
    case INTERACTION_SELECTING:
        m_dragCurrentPos = event->localPos();
        update();
        break;
    case INTERACTION_BEGIN_MOVING:
        m_currentInteractionState = INTERACTION_MOVING;
    case INTERACTION_MOVING:
        m_dragCurrentPos = event->localPos();
        updateGeometry();
        break;
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
            m_currentInteractionState =
                updateSelection(mergeSelection) ? INTERACTION_SELECTED
                                                : INTERACTION_NONE;
        }
        break;
    case INTERACTION_BEGIN_MOVING:
        m_currentInteractionState = INTERACTION_SELECTED;
        break;
    case INTERACTION_MOVING:
        m_currentInteractionState = INTERACTION_SELECTED;
        applyManipulation();
        updateGeometry();
        m_dragOriginPos = m_dragCurrentPos;
        break;
    case INTERACTION_NONE:
    case INTERACTION_SELECTED:
        return; // should not be reached
    }
}

bool Scatterplot::updateSelection(bool mergeSelection)
{
    QSet<int> selection;
    if (mergeSelection) {
        selection.unite(m_selectedGlyphs);
    }

    m_sx.inverse();
    m_sy.inverse();

    float originX  = m_sx(m_dragOriginPos.x());
    float originY  = m_sy(m_dragOriginPos.y());
    float currentX = m_sx(m_dragCurrentPos.x());
    float currentY = m_sy(m_dragCurrentPos.y());

    m_sy.inverse();
    m_sx.inverse();

    QRectF selectionRect(QPointF(originX, originY), QPointF(currentX, currentY));

    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        const arma::rowvec &row = m_xy.row(i);

        if (selectionRect.contains(row[0], row[1])) {
            selection.insert(i);
        }
    }

    setSelection(selection);
    return !selection.isEmpty();
}

void Scatterplot::setSelection(const QSet<int> &selection)
{
    m_selectedGlyphs = selection;
    update();

    emit selectionChanged(selection);
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

    for (auto it = m_selectedGlyphs.cbegin(); it != m_selectedGlyphs.cend(); it++) {
        arma::rowvec row = m_xy.row(*it);
        row[0] = rx(m_sx(row[0]) + tx);
        row[1] = ry(m_sy(row[1]) + ty);
        m_xy.row(*it) = row;
    }

    emit xyInteractivelyChanged(m_xy);
}
