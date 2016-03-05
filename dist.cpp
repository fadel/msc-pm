#include "mp.h"

#include "utils.h"

double mp::euclidean(const arma::rowvec &x1, const arma::rowvec &x2)
{
    return arma::norm(x1 - x2, 2);
}

arma::mat mp::dist(const arma::mat &X, mp::DistFunc dfunc)
{
    int n = uintToInt<arma::uword, int>(X.n_rows);
    arma::mat D(n, n);

    #pragma omp parallel for shared(X, D, n)
    for (int i = 0; i < n; i++) {
        D(i, i) = 0;
        for (int j = 0; j < i; j++) {
            D(i, j) = dfunc(X.row(i), X.row(j));
            D(j, i) = D(i, j);
        }
    }

    return D;
}
