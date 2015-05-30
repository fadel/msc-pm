#include "scatterplot.h"

#include <cmath>

static const qreal GLYPH_OPACITY = 0.3;
static const qreal GLYPH_OPACITY_SELECTED = 1.0;
static const QColor SELECTION_COLOR(QColor(128, 128, 128, 96));
static const int GLYPH_SIZE = 8;
static const float PADDING = 10;
static const float PI = 3.1415f;

Scatterplot::Scatterplot(QQuickItem *parent)
    : QQuickItem(parent)
    , m_colorScale{
        QColor("#1f77b4"),
        QColor("#ff7f0e"),
        QColor("#2ca02c"),
        QColor("#d62728"),
        QColor("#9467bd"),
        QColor("#8c564b"),
        QColor("#e377c2"),
        QColor("#17becf"),
        QColor("#7f7f7f"),
    }
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
}

Scatterplot::~Scatterplot()
{
}

void Scatterplot::setData(const arma::mat &data)
{
    if (data.n_cols != 3)
        return;

    m_data = data;
    m_xmin = data.col(0).min();
    m_xmax = data.col(0).max();
    m_ymin = data.col(1).min();
    m_ymax = data.col(1).max();

    m_colorScale.setExtents(m_data.col(2).min(), m_data.col(2).max());
    m_selectedGlyphs.clear();

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

        // Place the glyph geometry node under an opacity node
        QSGOpacityNode *glyphOpacityNode = new QSGOpacityNode;
        glyphOpacityNode->appendChildNode(glyphNode);
        node->appendChildNode(glyphOpacityNode);
    }

    return node;
}

QSGNode *Scatterplot::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_data.n_rows < 1)
        return 0;

    qreal x, y, xt, yt, moveTranslationF;

    QSGNode *root = 0;
    if (!oldNode) {
        root = new QSGNode;
        root->appendChildNode(newGlyphNodeTree());
    } else {
        root = oldNode;
    }

    if (m_currentState != INTERACTION_MOVING)
        xt = yt = 0;
    else {
        xt = m_dragCurrentPos.x() - m_dragOriginPos.x();
        yt = m_dragCurrentPos.y() - m_dragOriginPos.y();
    }

    QSGNode *node = root->firstChild()->firstChild();
    for (arma::uword i = 0; i < m_data.n_rows; i++) {
        arma::rowvec row = m_data.row(i);
        bool isSelected = m_selectedGlyphs.contains(i);

        QSGOpacityNode *glyphOpacityNode = static_cast<QSGOpacityNode *>(node);
        glyphOpacityNode->setOpacity(isSelected ? GLYPH_OPACITY_SELECTED : GLYPH_OPACITY);

        QSGGeometryNode *glyphNode = static_cast<QSGGeometryNode *>(node->firstChild());
        QSGGeometry *geometry = glyphNode->geometry();
        moveTranslationF = isSelected ? 1.0 : 0.0;
        x = fromDataXToScreenX(row[0]) + xt * moveTranslationF;
        y = fromDataYToScreenY(row[1]) + yt * moveTranslationF;
        updateCircleGeometry(geometry, GLYPH_SIZE, x, y);
        glyphNode->markDirty(QSGNode::DirtyGeometry);

        node = node->nextSibling();
    }

    // Draw selection
    if (m_currentState == INTERACTION_SELECTING) {
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
    case INTERACTION_SELECTED:
        m_currentState = (event->button() == Qt::MiddleButton) ? INTERACTION_MOVING
                                                               : INTERACTION_SELECTING;
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
    switch (m_currentState) {
    case INTERACTION_SELECTING:
        m_dragCurrentPos = event->localPos();
        update();
        break;
    case INTERACTION_MOVING:
        m_dragCurrentPos = event->localPos();
        updateData();
        update();
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
    bool mergeSelection;

    switch (m_currentState) {
    case INTERACTION_SELECTING:
        mergeSelection = (event->button() == Qt::RightButton);
        m_currentState = selectGlyphs(mergeSelection) ? INTERACTION_SELECTED
                                                      : INTERACTION_NONE;
        update();
        break;

    case INTERACTION_MOVING:
        m_currentState = INTERACTION_SELECTED;
        update();
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
    for (arma::uword i = 0; i < m_data.n_rows; i++) {
        arma::rowvec row = m_data.row(i);
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

void Scatterplot::updateData()
{
    float xt = m_dragCurrentPos.x() - m_dragOriginPos.x();
    float yt = m_dragCurrentPos.y() - m_dragOriginPos.y();

    xt /= (width()  - PADDING);
    yt /= (height() - PADDING);
    for (auto it = m_selectedGlyphs.cbegin(); it != m_selectedGlyphs.cend(); it++) {
        arma::rowvec row = m_data.row(*it);
        row[0] = ((row[0] - m_xmin) / (m_xmax - m_xmin) + xt) * (m_xmax - m_xmin) + m_xmin;
        row[1] = ((row[1] - m_ymin) / (m_ymax - m_ymin) + yt) * (m_ymax - m_ymin) + m_ymin;
        m_data.row(*it) = row;
    }

    // does not send last column (labels)
    emit dataChanged(m_data.cols(0, m_data.n_cols - 2));
}
