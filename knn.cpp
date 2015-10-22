#include "mp.h"

void mp::knn(const arma::mat &dmat, arma::uword i, arma::uword k, arma::uvec &nn, arma::vec &dist)
{
    arma::uword n = dist.n_rows;
    double dmax = arma::datum::inf;
    nn.fill(i);
    dist.fill(dmax);
    if (k > n) {
        return;
    }

    const arma::vec &dvec = dmat.col(i);
    for (arma::uword j = 0; j < n; j++) {
        if (j == i || dvec[j] > dmax) {
            continue;
        }

        arma::uword l;
        for (l = 0; dist[l] < dvec[j] && l < k; l++);
        for (arma::uword m = k - 1; m > l; m--) {
            nn[m]   = nn[m - 1];
            dist[m] = dist[m - 1];
        }

        nn[l] = j;
        dist[l] = dvec[j];
        dmax = dist[k - 1];
    }
}
