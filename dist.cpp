#include "mp.h"

double mp::euclidean(const arma::rowvec &x1, const arma::rowvec &x2)
{
    return arma::norm(x1 - x2, 2);
}

arma::mat mp::dist(const arma::mat &X, double (*distCalc)(const arma::rowvec &, const arma::rowvec &))
{
    arma::uword n = X.n_rows;
    arma::mat D(n, n, arma::fill::zeros);

    for (arma::uword i = 0; i < n; i++)
        for (arma::uword j = 0; j < i; j++) {
            D(i, j) = distCalc(X.row(i), X.row(j));
            D(j, i) = D(i, j);
        }

    return D;
}
