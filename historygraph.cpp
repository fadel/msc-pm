#include "historygraph.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>

#include "geometry.h"
#include "scale.h"

static const float ASPECT = 4.f / 3.f;
static const float MARGIN = 0.1f;
static const float PADDING = 0.05f;

static const float GLYPH_SIZE = 4.0f;
static const float GLYPH_OPACITY = 0.6f;

class HistoryGraph::HistoryItemNode
{
public:
    HistoryItemNode(const arma::mat &item);
    ~HistoryItemNode();

    void setNext(HistoryItemNode *node);
    HistoryItemNode *next() const { return m_next; }

    const arma::mat &item() const;

    const QRectF &rect() const { return m_rect; }
    void setRect(const QRectF &rect) { m_rect = rect; }

    int length() const { return m_length; }

private:
    void updateLength();

    arma::mat m_item;
    HistoryItemNode *m_prev, *m_next;
    int m_length;
    QRectF m_rect;
};

HistoryGraph::HistoryItemNode::HistoryItemNode(const arma::mat &item)
    : m_item(item)
    , m_prev(0)
    , m_next(0)
    , m_length(1)
{
}

HistoryGraph::HistoryItemNode::~HistoryItemNode()
{
    m_prev = 0;

    if (m_next) {
        delete m_next;
    }
}

void HistoryGraph::HistoryItemNode::updateLength()
{
    m_length = 1 + (m_next ? m_next->length() : 0);

    if (m_prev) {
        m_prev->updateLength();
    }
}

void HistoryGraph::HistoryItemNode::setNext(HistoryItemNode *node)
{
    if (!node) {
        return;
    }

    if (m_next) {
        delete m_next;
    }

    m_next = node;
    node->m_prev = this;
}

const arma::mat &HistoryGraph::HistoryItemNode::item() const
{
    return m_item;
}

HistoryGraph::HistoryGraph(QQuickItem *parent)
    : QQuickItem(parent)
    , m_firstNode(0)
    , m_currentNode(0)
    , m_currentWidth(0.0f)
    , m_needsUpdate(false)
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

HistoryGraph::~HistoryGraph()
{
    delete m_firstNode;
}

void HistoryGraph::addHistoryItem(const arma::mat &item)
{
    HistoryItemNode *newNode = new HistoryItemNode(item);
    if (m_currentNode) {
        m_currentNode->setNext(newNode);
    } else {
        m_firstNode = newNode;
    }

    m_currentNode = newNode;
    m_needsUpdate = true;
    update();
}

void HistoryGraph::addScatterplot(QSGNode *node, const HistoryGraph::HistoryItemNode *historyItemNode, float x, float y, float w, float h)
{
    const arma::mat &xy = historyItemNode->item();
    int vertexCount = calculateCircleVertexCount(GLYPH_SIZE / 2);

    LinearScale sx(xy.col(0).min(), xy.col(0).max(), x, x + w);
    LinearScale sy(xy.col(1).min(), xy.col(1).max(), y + h, y); // reverse on purpose

    for (arma::uword i = 0; i < xy.n_rows; i++) {
        const arma::rowvec &row = xy.row(i);
        QSGGeometryNode *glyphNode = new QSGGeometryNode;

        QSGGeometry *glyphGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
        glyphGeometry->setDrawingMode(GL_POLYGON);
        updateCircleGeometry(glyphGeometry, GLYPH_SIZE / 2 - 0.5, sx(row[0]), sy(row[1]));
        glyphNode->setGeometry(glyphGeometry);
        glyphNode->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(QColor());
        glyphNode->setMaterial(material);
        glyphNode->setFlag(QSGNode::OwnsMaterial);

        // Place the glyph geometry node under an opacity node
        QSGOpacityNode *glyphOpacityNode = new QSGOpacityNode;
        glyphOpacityNode->setOpacity(GLYPH_OPACITY);
        glyphOpacityNode->appendChildNode(glyphNode);
        node->appendChildNode(glyphOpacityNode);
    }
}

QSGNode *HistoryGraph::createNodeTree()
{
    if (!m_firstNode) {
        return 0;
    }

    //int breadth = m_firstNode->breadth();
    //int depth   = m_firstNode->depth();

    QSGTransformNode *sceneGraphRoot = new QSGTransformNode;
    float margin = height()*MARGIN;
    float padding = height()*PADDING;
    float h = height() - 2.f*margin;
    float w = ASPECT * h;
    float x = margin;

    QMatrix4x4 mat;
    for (HistoryItemNode *histNode = m_firstNode; histNode; histNode = histNode->next()) {
        QSGOpacityNode *opacityNode = new QSGOpacityNode;
        opacityNode->setOpacity(histNode == m_currentNode ? 1.0f : 0.4f);

        histNode->setRect(QRectF(x, margin, w, h));
        QSGGeometryNode *histItemGeomNode = new QSGGeometryNode;
        QSGGeometry *histItemGeom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
        updateRectGeometry(histItemGeom, x, margin, w, h);
        histItemGeom->setDrawingMode(GL_LINE_LOOP);
        histItemGeomNode->setGeometry(histItemGeom);
        histItemGeomNode->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(QColor());
        histItemGeomNode->setMaterial(material);
        histItemGeomNode->setFlag(QSGNode::OwnsMaterial);

        addScatterplot(histItemGeomNode, histNode, x + padding, margin + padding, w - 2*padding, h - 2*padding);
        opacityNode->appendChildNode(histItemGeomNode);
        sceneGraphRoot->appendChildNode(opacityNode);

        x += w + 2.f*margin;
    }

    m_currentWidth = x - 2.0f*margin;

    if (m_currentWidth > width()) {
        const QRectF &rect = m_viewportTransform.mapRect(m_currentNode->rect());
        if (rect.x() < 0) {
            m_viewportTransform.translate(rect.x(), 0);
        } else if (rect.x() + rect.width() > width()) {
            m_viewportTransform.translate(width() - (rect.x() + rect.width()) - margin, 0);
        }
    } else {
        m_viewportTransform.setToIdentity();
    }

    return sceneGraphRoot;
}

void HistoryGraph::updateNodeTree(QSGNode *root)
{
    if (!m_firstNode) {
        return;
    }

    // FIXME: (really) lame update algorithm

    QSGNode *child = root->firstChild();
    if (child) {
        root->removeChildNode(child);
        delete child;
    }

    root->appendChildNode(createNodeTree());
}

QSGNode *HistoryGraph::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = 0;
    if (!oldNode) {
        root = new QSGNode;
        QSGNode *node = createNodeTree();
        if (node) {
            root->appendChildNode(node);
        }
    } else {
        root = oldNode;
    }

    if (m_needsUpdate) {
        m_needsUpdate = false;
        updateNodeTree(root);
    }

    static_cast<QSGTransformNode *>(root->firstChild())->setMatrix(m_viewportTransform);

    return root;
}

HistoryGraph::HistoryItemNode *HistoryGraph::nodeAt(const QPointF &pos) const
{
    return nodeAt(pos, m_firstNode);
}

HistoryGraph::HistoryItemNode *HistoryGraph::nodeAt(const QPointF &pos, HistoryGraph::HistoryItemNode *node) const
{
    if (!node) {
        return 0;
    }

    const QRectF &rect = m_viewportTransform.mapRect(node->rect());
    if (pos.x() < rect.x()) {
        return 0;
    }
    if (rect.contains(pos)) {
        return node;
    }

    HistoryGraph::HistoryItemNode *tmp = nodeAt(pos, node->next());
    if (tmp) {
        return tmp;
    }

    return 0;
}

static inline float clamp(float x, float min, float max)
{
    if (x < min) {
        return min;
    } else if (x > max) {
        return max;
    }

    return x;
}

void HistoryGraph::hoverMoveEvent(QHoverEvent *event)
{
    if (m_currentWidth < width()) {
        return;
    }

    const QPointF &pos = event->posF();
    float margin = MARGIN * height();
    float prop = (pos.x() - 2*margin) / (width() - 4*margin);
    prop = clamp(prop, 0.0f, 1.0f);
    float displ = m_currentWidth - width() + margin;

    m_viewportTransform.setToIdentity();
    m_viewportTransform.translate(-displ * prop, 0);

    update();
}

void HistoryGraph::mousePressEvent(QMouseEvent *event)
{
    HistoryGraph::HistoryItemNode *node = nodeAt(event->localPos());
    if (!node || node == m_currentNode) {
        event->ignore();
        return;
    }

    m_currentNode = node;
    m_needsUpdate = true;
    update();

    emit currentItemChanged(m_currentNode->item());
}
