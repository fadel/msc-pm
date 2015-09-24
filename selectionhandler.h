#ifndef SELECTIONHANDLER_H
#define SELECTIONHANDLER_H

#include <QObject>
#include <QSet>
#include <armadillo>

class SelectionHandler : public QObject
{
    Q_OBJECT
public:
    SelectionHandler(const arma::uvec &sampleIndices);

signals:
    void selectionChanged(const QSet<int> &selection);

public slots:
    void setSelection(const QSet<int> &selection);

private:
    arma::uvec m_sampleIndices;
};

#endif // SELECTIONHANDLER_H
