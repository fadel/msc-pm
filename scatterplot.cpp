#include "scatterplot.h"

#include "geometry.h"
#include <cmath>

static const qreal GLYPH_OPACITY = 0.4;
static const qreal GLYPH_OPACITY_SELECTED = 1.0;

static const QColor OUTLINE_COLOR(0, 0, 0);
static const QColor SELECTION_COLOR(128, 128, 128, 96);

static const int GLYPH_SIZE = 8.f;
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

    float min = std::min(m_xy.col(0).min(), m_oldXY.col(0).max());
    float max = std::max(m_xy.col(0).max(), m_oldXY.col(0).max());
    m_sx.setDomain(min, max);
    min = std::min(m_xy.col(1).min(), m_oldXY.col(1).max());
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

QSGNode *Scatterplot::createGlyphNodeTree()
{
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

QSGNode *Scatterplot::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = 0;
    if (!oldNode) {
        root = new QSGNode;
        QSGNode *glyphTreeRoot = createGlyphNodeTree();
        if (glyphTreeRoot) {
            root->appendChildNode(glyphTreeRoot);
        }
    } else {
        root = oldNode;
    }

    if (m_xy.n_rows < 1) {
        return root;
    }

    qreal x, y, tx, ty, moveTranslationF;

    if (m_currentInteractionState == INTERACTION_MOVING) {
        tx = m_dragCurrentPos.x() - m_dragOriginPos.x();
        ty = m_dragCurrentPos.y() - m_dragOriginPos.y();
    } else {
        tx = ty = 0;
    }

    m_sx.setRange(PADDING, width() - 2*PADDING);
    m_sy.setRange(height() - 2*PADDING, PADDING);

    QSGNode *node = root->firstChild()->firstChild();
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

    if (m_shouldUpdateGeometry) {
        m_shouldUpdateGeometry = false;
    }
    if (m_shouldUpdateMaterials) {
        m_shouldUpdateMaterials = false;
    }

    // Selection rect
    if (m_currentInteractionState == INTERACTION_SELECTING) {
        QSGSimpleRectNode *selectionNode = 0;
        if (!root->firstChild()->nextSibling()) {
            selectionNode = new QSGSimpleRectNode;
            selectionNode->setColor(SELECTION_COLOR);
            root->appendChildNode(selectionNode);
        } else {
            selectionNode = static_cast<QSGSimpleRectNode *>(root->firstChild()->nextSibling());
        }

        selectionNode->setRect(QRectF(m_dragOriginPos, m_dragCurrentPos));
        selectionNode->markDirty(QSGNode::DirtyGeometry);
    } else {
        node = root->firstChild()->nextSibling();
        if (node) {
            root->removeChildNode(node);
            delete node;
        }
    }

    animationTick();
    return root;
}

void Scatterplot::resetAnimation()
{
    m_t = 0;
}

void Scatterplot::startAnimation()
{
    resetAnimation();
    update();
}

void Scatterplot::animationTick()
{
    if (m_t < 1.f) {
        m_t += 0.1f;
        updateGeometry();
    }
}

void Scatterplot::mousePressEvent(QMouseEvent *event)
{
    switch (m_currentInteractionState) {
    case INTERACTION_NONE:
    case INTERACTION_SELECTED:
        if (event->modifiers() == Qt::AltModifier) {
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
        return;
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

    m_sx.reverse();
    m_sy.reverse();

    float originX  = m_sx(m_dragOriginPos.x());
    float originY  = m_sy(m_dragOriginPos.y());
    float currentX = m_sx(m_dragCurrentPos.x());
    float currentY = m_sy(m_dragCurrentPos.y());

    m_sy.reverse();
    m_sx.reverse();

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
    m_sx.reverse();
    m_sy.reverse();
    LinearScale rx = m_sx;
    LinearScale ry = m_sy;
    m_sy.reverse();
    m_sx.reverse();

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
