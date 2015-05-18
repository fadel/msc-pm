#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include <armadillo>
#include <vector>
#include <QQuickItem>

#include "colorscale.h"

class Scatterplot : public QQuickItem
{
    Q_OBJECT
public:
    Scatterplot(QQuickItem *parent = 0);
    ~Scatterplot();

    void setData(const arma::mat &data);

signals:

public slots:

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

private:
    arma::mat m_data;
    ColorScale m_colorScale;
};

#endif // SCATTERPLOT_H
