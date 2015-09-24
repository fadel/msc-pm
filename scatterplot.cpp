#include "scatterplot.h"

#include <cmath>

static const qreal GLYPH_OPACITY = 0.4;
static const qreal GLYPH_OPACITY_SELECTED = 1.0;

static const QColor OUTLINE_COLOR(0, 0, 0);
static const QColor SELECTION_COLOR(128, 128, 128, 96);

static const int GLYPH_SIZE = 8;
static const float PADDING = 10;
static const float PI = 3.1415f;

Scatterplot::Scatterplot(QQuickItem *parent)
    : QQuickItem(parent)
    , m_currentInteractionState(INTERACTION_NONE)
    , m_shouldUpdateGeometry(false)
    , m_shouldUpdateMaterials(false)
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
        m_selectedGlyphs.clear();
    }

    m_xy = xy;
    m_xmin = xy.col(0).min();
    m_xmax = xy.col(0).max();
    m_ymin = xy.col(1).min();
    m_ymax = xy.col(1).max();

    updateGeometry();

    emit xyChanged(m_xy);
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

static int calculateCircleVertexCount(qreal radius)
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

inline float Scatterplot::fromDataXToScreenX(float x) const
{
    return PADDING + (x - m_xmin) / (m_xmax - m_xmin) * (width() - 2*PADDING);
}

inline float Scatterplot::fromDataYToScreenY(float y) const
{
    return PADDING + (1 - (y - m_ymin) / (m_ymax - m_ymin)) * (height() - 2*PADDING);
}

QSGNode *Scatterplot::createGlyphNodeTree()
{
    QSGNode *node = new QSGNode;
    int vertexCount = calculateCircleVertexCount(GLYPH_SIZE / 2);

    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        QSGGeometry *glyphOutlineGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        glyphOutlineGeometry->setDrawingMode(GL_LINE_LOOP);
        updateCircleGeometry(glyphOutlineGeometry, GLYPH_SIZE, 0, 0);
        QSGGeometryNode *glyphOutlineNode = new QSGGeometryNode;
        glyphOutlineNode->setGeometry(glyphOutlineGeometry);
        glyphOutlineNode->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(OUTLINE_COLOR);
        glyphOutlineNode->setMaterial(material);
        glyphOutlineNode->setFlag(QSGNode::OwnsMaterial);

        QSGGeometry *glyphGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        glyphGeometry->setDrawingMode(GL_POLYGON);
        updateCircleGeometry(glyphGeometry, GLYPH_SIZE - 1, 0, 0);
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
    if (m_xy.n_rows < 1) {
        return 0;
    }

    qreal x, y, tx, ty, moveTranslationF;

    QSGNode *root = 0;
    if (!oldNode) {
        root = new QSGNode;
        root->appendChildNode(createGlyphNodeTree());
    } else {
        root = oldNode;
    }

    if (m_currentInteractionState == INTERACTION_MOVING) {
        tx = m_dragCurrentPos.x() - m_dragOriginPos.x();
        ty = m_dragCurrentPos.y() - m_dragOriginPos.y();
    } else {
        tx = ty = 0;
    }

    QSGNode *node = root->firstChild()->firstChild();
    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        arma::rowvec row = m_xy.row(i);
        bool isSelected = m_selectedGlyphs.contains(i);

        QSGOpacityNode *glyphOpacityNode = static_cast<QSGOpacityNode *>(node);
        glyphOpacityNode->setOpacity(isSelected ? GLYPH_OPACITY_SELECTED : GLYPH_OPACITY);

        QSGGeometryNode *glyphOutlineNode = static_cast<QSGGeometryNode *>(node->firstChild());
        QSGGeometryNode *glyphNode = static_cast<QSGGeometryNode *>(node->firstChild()->nextSibling());
        if (m_shouldUpdateGeometry) {
            moveTranslationF = isSelected ? 1.0 : 0.0;
            x = fromDataXToScreenX(row[0]) + tx * moveTranslationF;
            y = fromDataYToScreenY(row[1]) + ty * moveTranslationF;

            QSGGeometry *geometry = glyphOutlineNode->geometry();
            updateCircleGeometry(geometry, GLYPH_SIZE, x, y);
            glyphOutlineNode->markDirty(QSGNode::DirtyGeometry);

            geometry = glyphNode->geometry();
            updateCircleGeometry(geometry, GLYPH_SIZE - 1, x, y);
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
            // node->markDirty(QSGNode::DirtyGeometry);
            root->removeChildNode(node);
            delete node;
        }
    }

    return root;
}

void Scatterplot::mousePressEvent(QMouseEvent *event)
{
    switch (m_currentInteractionState) {
    case INTERACTION_NONE:
    case INTERACTION_SELECTED:
        if (event->modifiers() == Qt::AltModifier) {
            m_currentInteractionState = INTERACTION_MOVING;
        } else {
            m_currentInteractionState = INTERACTION_SELECTING;
        }
        m_dragOriginPos = event->localPos();
        m_dragCurrentPos = m_dragOriginPos;
        break;
    case INTERACTION_SELECTING:
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
    case INTERACTION_MOVING:
        m_dragCurrentPos = event->localPos();
        applyManipulation();
        updateGeometry();
        m_dragOriginPos = m_dragCurrentPos;
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

    case INTERACTION_MOVING:
        m_currentInteractionState = INTERACTION_SELECTED;
        updateGeometry();
        emit xyChanged(m_xy);
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

    qreal originX  = m_dragOriginPos.x() / width() * (m_xmax - m_xmin) + m_xmin;
    qreal originY  = (1 - m_dragOriginPos.y() / height()) * (m_ymax - m_ymin) + m_ymin;
    qreal currentX = m_dragCurrentPos.x() / width() * (m_xmax - m_xmin) + m_xmin;
    qreal currentY = (1 - m_dragCurrentPos.y() / height()) * (m_ymax - m_ymin) + m_ymin;

    QRectF selectionRect(QPointF(originX, originY), QPointF(currentX, currentY));

    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        arma::rowvec row = m_xy.row(i);

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
    float tx = m_dragCurrentPos.x() - m_dragOriginPos.x();
    float ty = m_dragCurrentPos.y() - m_dragOriginPos.y();

    tx /=  (width()  - PADDING);
    ty /= -(height() - PADDING);

    float x_extent = m_xmax - m_xmin;
    float y_extent = m_ymax - m_ymin;

    for (auto it = m_selectedGlyphs.cbegin(); it != m_selectedGlyphs.cend(); it++) {
        arma::rowvec row = m_xy.row(*it);
        row[0] = ((row[0] - m_xmin) / x_extent + tx) * x_extent + m_xmin;
        row[1] = ((row[1] - m_ymin) / y_extent + ty) * y_extent + m_ymin;
        m_xy.row(*it) = row;
    }
}
