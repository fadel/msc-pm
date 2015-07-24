#include "npdistortion.h"

#include "mp.h"

NPDistortion::NPDistortion(const arma::mat &X, const arma::uvec &sampleIndices, int k)
    : DistortionObserver(X, sampleIndices)
    , m_k(k)
{
}

arma::vec NPDistortion::measureFunc(const arma::mat &distA, const arma::mat &distB)
{
    return mp::neighborhoodPreservation(distA, distB, m_k);
}
