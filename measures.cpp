#include "mp.h"

#include <algorithm>

static
void knn(const arma::mat &dmat, arma::uword i, arma::uword k, arma::uvec &nn, arma::vec &dist)
{
    arma::uword n = dist.n_rows;
    if (k > n) {
        return;
    }

    for (arma::uword j = 0, l = 0; l < k; j++, l++) {
        if (j == i) {
            j++;
        }

        nn[l] = j;
        dist[l] = dmat(i, j);
    }

    double dmax = *std::max_element(dist.begin(), dist.end());
    for (arma::uword j = 0; j < n; j++) {
        if (j == i) {
            continue;
        }

        if (dmat(i, j) < dmax) {
            dmax = dmat(i, j);
            arma::uword l;
            for (l = 0; dmat(i, j) > dist[l] && l < k; l++);
            for (arma::uword m = l + 1; m < k; m++) {
                nn[m]   = nn[m - 1];
                dist[m] = dist[m - 1];
            }

            nn[l] = j;
            dist[l] = dmat(i, j);
        }
    }
}

arma::vec mp::neighborhoodPreservation(const arma::mat &distA,
                                       const arma::mat &distB,
                                       arma::uword k)
{
    arma::uword n = distA.n_rows;
    arma::vec np(n);

    #pragma omp parallel for shared(np, n)
    for (arma::uword i = 0; i < n; i++) {
        arma::uvec nnA(k);
        arma::uvec nnB(k);
        arma::vec dist(k);

        knn(distA, i, k, nnA, dist);
        knn(distB, i, k, nnB, dist);

        std::sort(nnA.begin(), nnA.end());
        std::sort(nnB.begin(), nnB.end());

        arma::uword l;
        for (l = 0; nnA[l] == nnB[l] && l < k; l++);
        np[i] = ((double) l) / k;
    }

    return np;
}

arma::vec mp::silhouette(const arma::mat &distA,
                         const arma::mat &distB,
                         const arma::vec &labels)
{
    // TODO
    return arma::vec(distA.n_rows, arma::fill::zeros);
}
