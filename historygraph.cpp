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

    void append(HistoryItemNode *node);
    const QList<HistoryItemNode *> &children() const { return m_next; }

    const arma::mat &item() const;
    int depth() const   { return m_depth; }
    int breadth() const { return m_breadth; }
    const QRectF &rect() const { return m_rect; }

    void setRect(const QRectF &rect) { m_rect = rect; }

    void updateDepth();
    void updateBreadth();

    static int treeBreadth(HistoryItemNode *node, int level);

private:
    arma::mat m_item;
    HistoryItemNode *m_prev;
    QList<HistoryItemNode *> m_next;
    int m_depth, m_breadth;
    QRectF m_rect;
};

HistoryGraph::HistoryItemNode::HistoryItemNode(const arma::mat &item)
    : m_item(item)
    , m_prev(0)
    , m_depth(1)
    , m_breadth(1)
{
}

HistoryGraph::HistoryItemNode::~HistoryItemNode()
{
    m_prev = 0;

    while (!m_next.isEmpty()) {
        delete m_next.takeLast();
    }
}

void HistoryGraph::HistoryItemNode::updateDepth()
{
    HistoryGraph::HistoryItemNode *node = *std::max_element(
        m_next.cbegin(), m_next.cend(),
        [](const HistoryGraph::HistoryItemNode *n1, const HistoryGraph::HistoryItemNode *n2) {
            return n1->depth() < n2->depth();
        });

    m_depth = 1 + node->depth();

    if (m_prev) {
        m_prev->updateDepth();
    }
}

int HistoryGraph::HistoryItemNode::treeBreadth(HistoryGraph::HistoryItemNode *node, int level) {
    if (!node || level < 1) {
        return 0;
    }

    if (level == 1) {
        return 1;
    }

    return std::accumulate(node->m_next.cbegin(), node->m_next.cend(), 0, [](int b, HistoryGraph::HistoryItemNode *n) {
        n->m_breadth = treeBreadth(n, n->depth());
        return b + n->m_breadth;
    });
}

void HistoryGraph::HistoryItemNode::updateBreadth()
{
    HistoryItemNode *node = this;
    while (node->m_prev) {
        node = node->m_prev;
    }

    node->m_breadth = treeBreadth(node, node->m_depth);
}

void HistoryGraph::HistoryItemNode::append(HistoryItemNode *node)
{
    if (!node) {
        return;
    }

    m_next.append(node);
    if (node->depth() + 1 > m_depth) {
        updateDepth();
    }

    updateBreadth();

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
    , m_needsUpdate(false)
{
    setClip(true);
    setFlag(QQuickItem::ItemHasContents);
    setAcceptedMouseButtons(Qt::LeftButton);
}

HistoryGraph::~HistoryGraph()
{
    delete m_firstNode;
    m_firstNode = 0;
}

void HistoryGraph::addHistoryItem(const arma::mat &item)
{
    HistoryItemNode *newNode = new HistoryItemNode(item);
    if (m_currentNode) {
        m_currentNode->append(newNode);
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

    QSGNode *sceneGraphRoot = new QSGNode;
    HistoryItemNode *histNode = m_firstNode;
    float margin = height()*MARGIN;
    float padding = height()*PADDING;
    float h = height() - 2.f*margin;
    float w = ASPECT * h;
    float x = margin;

    QMatrix4x4 mat;
    do {
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

        QList<HistoryItemNode *> children = histNode->children();
        if (children.isEmpty()) {
            break;
        }

        // FIXME: add children
        histNode = children[0];
    } while (true);

    // setWidth(xPos + radius + margin);

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

    const QRectF &rect = node->rect();

    if (pos.x() < rect.x()) {
        return 0;
    }

    if (rect.contains(pos)) {
        return node;
    }

    QList<HistoryGraph::HistoryItemNode *> children = node->children();
    for (auto it = children.begin(); it != children.end(); it++) {
        HistoryGraph::HistoryItemNode *tmp = nodeAt(pos, *it);
        if (tmp) {
            return tmp;
        }
    }

    return 0;
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
