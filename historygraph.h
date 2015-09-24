#ifndef HISTORYGRAPH_H
#define HISTORYGRAPH_H

#include <armadillo>
#include <QtQuick>

class HistoryGraph : public QQuickItem
{
    Q_OBJECT
public:
    HistoryGraph(QQuickItem *parent = 0);

public slots:
    void addHistoryItem(const int &item);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
};

#endif // HISTORYGRAPH_H
