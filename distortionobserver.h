#ifndef DISTORTIONOBSERVER_H
#define DISTORTIONOBSERVER_H

#include <QObject>
#include <armadillo>

#include "distortionmeasure.h"

class DistortionObserver : public QObject
{
    Q_OBJECT
public:
    DistortionObserver(const arma::mat &X, const arma::uvec &sampleIndices);
    void setMeasure(DistortionMeasure *measure);

signals:
    void mapChanged(const arma::vec &distortion);

public slots:
    void setMap(const arma::mat &Y);

private:
    arma::mat m_X, m_Y, m_distX;
    arma::uvec m_sampleIndices;
    DistortionMeasure *m_distortionMeasure;
    arma::vec m_measures;
};

#endif // DISTORTIONOBSERVER_H
