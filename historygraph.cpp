#include "historygraph.h"

HistoryGraph::HistoryGraph(QQuickItem *parent)
    : QQuickItem(parent)
{
}

void HistoryGraph::addHistoryItem(const int &item)
{}

QSGNode *HistoryGraph::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    return oldNode;
}
