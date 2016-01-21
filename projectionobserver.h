#ifndef PROJECTIONOBSERVER_H
#define PROJECTIONOBSERVER_H

#include <QObject>
#include <armadillo>

class ProjectionObserver
    : public QObject
{
    Q_OBJECT
public:
    static const int OBSERVER_CURRENT       = 0;
    static const int OBSERVER_DIFF_PREVIOUS = 1;
    static const int OBSERVER_DIFF_ORIGINAL = 2;

    ProjectionObserver(const arma::mat &X, const arma::uvec &cpIndices);

signals:
    void valuesChanged(const arma::vec &values) const;
    void cpValuesChanged(const arma::vec &values) const;
    void rpValuesChanged(const arma::vec &values) const;

public slots:
    void setMap(const arma::mat &Y);
    bool setType(int type);

private:
    bool emitValuesChanged() const;

    int m_type;
    arma::mat m_X, m_Y, m_origY, m_prevY;
    arma::mat m_distX, m_distY, m_origDistY, m_prevDistY;
    arma::uvec m_cpIndices, m_rpIndices;

    // TODO: one per implemented measure
    arma::vec m_values, m_prevValues, m_origValues;
};

#endif // PROJECTIONOBSERVER_H
