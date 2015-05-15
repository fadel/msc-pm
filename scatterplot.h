#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include <armadillo>
#include <vector>
#include <QQuickItem>

class Scatterplot : public QQuickItem
{
    Q_OBJECT
public:
    Scatterplot();
    ~Scatterplot();

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);
    void setData(const arma::mat &data);

signals:

public slots:

private:
    arma::mat m_data;
};

#endif // SCATTERPLOT_H
