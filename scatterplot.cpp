#include "scatterplot.h"

#include <QMatrix4x4>
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
    m_colorScale = new ColorScale{
        QColor("#1f77b4"),
        QColor("#ff7f0e"),
        QColor("#2ca02c"),
        QColor("#d62728"),
        QColor("#9467bd"),
        QColor("#8c564b"),
        QColor("#e377c2"),
        QColor("#17becf"),
        QColor("#7f7f7f"),
    };

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

void Scatterplot::setXY(const arma::mat &xy)
{
    if (xy.n_cols != 2) {
        return;
    }

    m_xy = xy;
    m_xmin = xy.col(0).min();
    m_xmax = xy.col(0).max();
    m_ymin = xy.col(1).min();
    m_ymax = xy.col(1).max();
    m_selectedGlyphs.clear();
    emit xyChanged(m_xy);

    updateGeometry();
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

inline float Scatterplot::fromDataXToScreenX(float x)
{
    return PADDING + (x - m_xmin) / (m_xmax - m_xmin) * (width() - 2*PADDING);
}

inline float Scatterplot::fromDataYToScreenY(float y)
{
    return PADDING + (y - m_ymin) / (m_ymax - m_ymin) * (height() - 2*PADDING);
}

QSGNode *Scatterplot::createGlyphNodeTree()
{
    QSGNode *node = new QSGNode;
    int vertexCount = calculateCircleVertexCount(GLYPH_SIZE / 2);

    m_glyphGeometryPtr.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount));
    QSGGeometry *glyphGeometry = m_glyphGeometryPtr.get();
    glyphGeometry->setDrawingMode(GL_POLYGON);
    updateCircleGeometry(glyphGeometry, GLYPH_SIZE, 0, 0);

    m_glyphOutlineGeometryPtr.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount));
    QSGGeometry *glyphOutlineGeometry = m_glyphOutlineGeometryPtr.get();
    glyphOutlineGeometry->setDrawingMode(GL_LINE_LOOP);
    updateCircleGeometry(glyphOutlineGeometry, GLYPH_SIZE + 1, 0, 0);

    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        QSGGeometryNode *glyphOutlineNode = new QSGGeometryNode;
        glyphOutlineNode->setGeometry(glyphOutlineGeometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(OUTLINE_COLOR);
        glyphOutlineNode->setMaterial(material);
        glyphOutlineNode->setFlag(QSGNode::OwnsMaterial);

        QSGGeometryNode *glyphNode = new QSGGeometryNode;
        glyphNode->setGeometry(glyphGeometry);

        material = new QSGFlatColorMaterial;
        material->setColor(QColor());
        glyphNode->setMaterial(material);
        glyphNode->setFlag(QSGNode::OwnsMaterial);

        QSGTransformNode *transformNode = new QSGTransformNode;
        transformNode->appendChildNode(glyphNode);
        transformNode->appendChildNode(glyphOutlineNode);

        // Place the glyph geometry node under an opacity node
        QSGOpacityNode *glyphOpacityNode = new QSGOpacityNode;
        glyphOpacityNode->appendChildNode(transformNode);
        node->appendChildNode(glyphOpacityNode);
    }

    return node;
}

QSGNode *Scatterplot::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_xy.n_rows < 1) {
        return 0;
    }

    QMatrix4x4 matrix;
    qreal tx, ty, moveTranslationF;

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

        QSGTransformNode *transformNode = static_cast<QSGTransformNode *>(node->firstChild());
        if (m_shouldUpdateGeometry) {
            moveTranslationF = isSelected ? 1.0 : 0.0;
            matrix(0, 3) = fromDataXToScreenX(row[0]) + tx * moveTranslationF;
            matrix(1, 3) = fromDataYToScreenY(row[1]) + ty * moveTranslationF;
            transformNode->setMatrix(matrix);
        }
        if (m_shouldUpdateMaterials) {
            QSGGeometryNode *glyphNode = static_cast<QSGGeometryNode *>(transformNode->firstChild());
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

    // Draw selection
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
            node->markDirty(QSGNode::DirtyGeometry);
            root->removeChildNode(node);
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
            INTERACTION_SELECTING;
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
        bool mergeSelection = (event->modifiers() == Qt::ControlModifier);
        if (selectGlyphs(mergeSelection)) {
            m_currentInteractionState = INTERACTION_SELECTED;
        else {
            m_currentInteractionState = INTERACTION_NONE;
        }
        update();
        break;

    case INTERACTION_MOVING:
        m_currentInteractionState = INTERACTION_SELECTED;
        updateGeometry();
        break;
    case INTERACTION_NONE:
    case INTERACTION_SELECTED:
        return; // should not be reached
    }
}

bool Scatterplot::selectGlyphs(bool mergeSelection)
{
    if (!mergeSelection)
        m_selectedGlyphs.clear();

    qreal x, y;

    QRectF selectionRect(m_dragOriginPos, m_dragCurrentPos);
    bool anySelected = false;
    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        arma::rowvec row = m_xy.row(i);
        x = fromDataXToScreenX(row[0]);
        y = fromDataYToScreenY(row[1]);

        if (selectionRect.contains(x, y)) {
            m_selectedGlyphs.insert(i);
            if (!anySelected)
                anySelected = true;
        }
    }

    return anySelected;
}

void Scatterplot::applyManipulation()
{
    float tx = m_dragCurrentPos.x() - m_dragOriginPos.x();
    float ty = m_dragCurrentPos.y() - m_dragOriginPos.y();

    tx /= (width()  - PADDING);
    ty /= (height() - PADDING);

    float x_extent = m_xmax - m_xmin;
    float y_extent = m_ymax - m_ymin;

    for (auto it = m_selectedGlyphs.cbegin(); it != m_selectedGlyphs.cend(); it++) {
        arma::rowvec row = m_xy.row(*it);
        row[0] = ((row[0] - m_xmin) / x_extent + tx) * x_extent + m_xmin;
        row[1] = ((row[1] - m_ymin) / y_extent + ty) * y_extent + m_ymin;
        m_xy.row(*it) = row;
    }

    emit xyChanged(m_xy);
}
