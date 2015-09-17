#ifndef SELECTIONHANDLER_H
#define SELECTIONHANDLER_H

#include <QObject>
#include <armadillo>

class SelectionHandler : public QObject
{
    Q_OBJECT
public:
    SelectionHandler(const arma::uvec &sampleIndices);

signals:
    void selectionChanged(const arma::uvec &selection);

public slots:
    void setSelection(const arma::uvec &selection);

private:
    arma::uvec m_sampleIndices;
};

#endif // SELECTIONHANDLER_H
