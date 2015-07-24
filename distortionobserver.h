#ifndef DISTORTIONOBSERVER_H
#define DISTORTIONOBSERVER_H

#include <QObject>
#include <armadillo>

class DistortionObserver : public QObject
{
    Q_OBJECT
public:
    DistortionObserver(const arma::mat &X, const arma::uvec &sampleIndices);
    virtual ~DistortionObserver();

signals:
    void mapChanged(const arma::vec &distortion);

public slots:
    void setMap(const arma::mat &Y);

protected:
    virtual arma::vec measureFunc(const arma::mat &distA, const arma::mat &distB) = 0;

private:
    arma::mat m_X, m_Y, m_distX;
    arma::uvec m_sampleIndices;
    arma::vec m_measures;
};

#endif // DISTORTIONOBSERVER_H
