#ifndef NPDISTORTION_H
#define NPDISTORTION_H

#include "distortionobserver.h"

class NPDistortion : public DistortionObserver
{
public:
    NPDistortion(const arma::mat &X, const arma::uvec &sampleIndices, int k = 10);

protected:
    arma::vec measureFunc(const arma::mat &distA, const arma::mat &distB);

private:
    int m_k;
};

#endif // NPDISTORTION_H
