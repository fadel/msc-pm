#include "npdistortion.h"

#include "mp.h"

NPDistortion::NPDistortion(int k)
    : m_k(k)
{
}

arma::vec NPDistortion::measure(const arma::mat &distA, const arma::mat &distB)
{
    return mp::neighborhoodPreservation(distA, distB, m_k);
}
