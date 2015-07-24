#include "mp.h"

arma::vec mp::neighborhoodPreservation(const arma::mat &distA,
                                       const arma::mat &distB,
                                       int k)
{
    // TODO
    return arma::vec(distA.n_rows, arma::fill::zeros);
}

arma::vec mp::silhouette(const arma::mat &distA,
                         const arma::mat &distB,
                         const arma::vec &labels)
{
    // TODO
    return arma::vec(distA.n_rows, arma::fill::zeros);
}
