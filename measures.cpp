#include "mp.h"

#include <cassert>
#include <cmath>
#include <algorithm>

#include "utils.h"

static const float EPSILON = 1e-6f;

void mp::neighborhoodPreservation(const arma::mat &distA,
                                  const arma::mat &distB,
                                  arma::uword k,
                                  arma::vec &v)
{
    int n = uintToInt<arma::uword, int>(v.n_elem);

    #pragma omp parallel for shared(v, n)
    for (int i = 0; i < n; i++) {
        //arma::uvec nnA(k);
        //arma::uvec nnB(k);
        //arma::vec dist(k);

        //mp::knn(distA, i, k, nnA, dist);
        //mp::knn(distB, i, k, nnB, dist);

        arma::uvec nnA = arma::sort_index(distA.col(i));
        nnA = nnA.subvec(2, k + 1);
        arma::uvec nnB = arma::sort_index(distB.col(i));
        nnB = nnB.subvec(2, k + 1);

        std::sort(nnA.begin(), nnA.end());
        std::sort(nnB.begin(), nnB.end());

        arma::uword l;
        for (l = 0; nnA[l] == nnB[l] && l < k; l++);
        v[i] = ((double) l) / k;
    }
}

arma::vec mp::silhouette(const arma::mat &distA,
                         const arma::mat &distB,
                         const arma::vec &labels)
{
    // TODO
    return arma::vec(distA.n_rows, arma::fill::zeros);
}

void mp::aggregatedError(const arma::mat &distX,
                         const arma::mat &distY,
                         arma::vec &v)
{
    int n = uintToInt<arma::uword, int>(v.n_elem);
    double maxX = distX.max();
    double maxY = distY.max();

    #pragma omp parallel for shared(maxX, maxY, distX, distY, v, n)
    for (int i = 0; i < n; i++) {
        v[i] = 0;
        for (int j = 0; j < n; j++) {
            if (i == j) {
                continue;
            }

            double diff = fabs(distY(i, j) / maxY - distX(i, j) / maxX);
            if (diff < EPSILON) {
                continue;
            }

            v[i] += diff;
        }
    }
}

/*
double mp::stress(const arma::mat &Dp, const arma::mat &Dq)
{
    assert(Dp.n_rows == Dp.n_cols);
    assert(Dq.n_rows == Dq.n_cols);
    assert(Dp.n_rows == Dq.n_rows);

    arma::uword n = Dp.n_rows;
    double sigma = 0, s = 0;
    for (arma::uword i = 0; i < n; i++)
        for (arma::uword j = i + 1; j < n; j++) {
            double delta = Dp(i, j);
            double d     = Dq(i, j);
            sigma += (delta - d) * (delta - d) / delta;
            s += delta;
        }

    return sigma / s;
}

arma::vec mp::klDivergence(const arma::mat &P, const arma::mat &Q)
{
    arma::vec diver(P.n_rows);
    mp::klDivergence(P, Q, diver);
    return diver;
}

void mp::klDivergence(const arma::mat &P, const arma::mat &Q, arma::vec &diverg)
{
    assert(P.n_rows == P.n_cols);
    assert(Q.n_rows == Q.n_cols);
    assert(P.n_rows == Q.n_cols);
    assert(diverg.n_elem == P.n_rows);

    arma::uword n = P.n_rows;
    for (arma::uword i = 0; i < n; i++)
        diverg(i) = klDivergence(P.row(i), Q.row(i));
}

double mp::klDivergence(const arma::rowvec &pi, const arma::rowvec &qi)
{
    // Pii and Qii should both be 1, zeroing the i-th term in the sum below
    return arma::accu(pi % arma::log(pi / qi));
}

arma::mat mp::d2p(const arma::mat &D, const arma::vec &sigmas)
{
    arma::mat P(D.n_rows, D.n_cols);
    mp::d2p(D, sigmas, P);
    return P;
}

void mp::d2p(const arma::mat &D, const arma::vec &sigmas, arma::mat &P)
{
    // WARNING: assumes D and sigmas are already squared
    assert(D.n_rows == D.n_cols);
    assert(P.n_rows == P.n_cols);
    assert(D.n_rows == P.n_rows);

    arma::uword n = D.n_rows;
    for (arma::uword i = 0; i < n; i++) {
        double den = -1; // k == i must be skipped
        for (arma::uword k = 0; k < n; k++)
            den += exp(-D(i, k) / sigmas(i));

        for (arma::uword j = 0; j < n; j++)
            P(i, j) = exp(-D(i, j) / sigmas(i)) / den;
    }
}
*/
