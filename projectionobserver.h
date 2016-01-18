#ifndef PROJECTIONOBSERVER_H
#define PROJECTIONOBSERVER_H

#include <QObject>
#include <armadillo>

#include "distortionmeasure.h"

class ProjectionObserver : public QObject
{
    Q_OBJECT
public:
    ProjectionObserver(const arma::mat &X, const arma::uvec &cpIndices);

signals:
    void mapChanged(const arma::vec &values) const;

public slots:
    void setMap(const arma::mat &Y);

private:
    arma::mat m_X, m_Y, m_origY, m_prevY;
    arma::mat m_distX, m_distY, m_distOrigY, m_distPrevY;
    arma::uvec m_cpIndices;

    // TODO: one per implemented measure
    arma::vec m_values;
};

#endif // PROJECTIONOBSERVER_H
