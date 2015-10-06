#ifndef HISTORYGRAPH_H
#define HISTORYGRAPH_H

#include <QtQuick>
#include <QMatrix4x4>
#include <armadillo>

class HistoryGraph : public QQuickItem
{
    Q_OBJECT
public:
    HistoryGraph(QQuickItem *parent = 0);
    ~HistoryGraph();

signals:
    void currentItemChanged(const arma::mat &item);

public slots:
    void addHistoryItem(const arma::mat &item);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void hoverMoveEvent(QHoverEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    class HistoryItemNode;

    HistoryItemNode *nodeAt(const QPointF &pos) const;
    HistoryItemNode *nodeAt(const QPointF &pos, HistoryItemNode *node) const;

    QSGNode *createNodeTree();
    void updateNodeTree(QSGNode *root);
    void addScatterplot(QSGNode *node, const HistoryItemNode *historyItemNode, float x, float y, float w, float h);

    HistoryItemNode *m_firstNode, *m_currentNode;
    QMatrix4x4 m_viewportTransform;
    float m_currentWidth;
    bool m_needsUpdate;
};

#endif // HISTORYGRAPH_H
