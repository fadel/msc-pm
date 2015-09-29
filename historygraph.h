#ifndef HISTORYGRAPH_H
#define HISTORYGRAPH_H

#include <QtQuick>
#include <armadillo>

class HistoryGraph : public QQuickItem
{
    Q_OBJECT
public:
    HistoryGraph(QQuickItem *parent = 0);
    ~HistoryGraph();

public slots:
    void addHistoryItem(const arma::mat &item);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

private:
    class HistoryItemNode;

    QSGNode *createNodeTree();
    void updateNodeTree(QSGNode *root);
    void addScatterplot(QSGNode *node, const HistoryItemNode *historyItemNode, float x, float y, float w, float h);


    HistoryItemNode *m_firstNode, *m_currentNode;
    bool m_needsUpdate;
};

#endif // HISTORYGRAPH_H
