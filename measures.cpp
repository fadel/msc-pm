#include "mp.h"

#include <cassert>
#include <cmath>
#include <algorithm>

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

        mp::knn(distA, i, k, nnA, dist);
        mp::knn(distB, i, k, nnB, dist);

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

void mp::klDivergence(const arma::mat &P, const arma::mat &Q, arma::vec &diver)
{
    assert(P.n_rows == P.n_cols);
    assert(Q.n_rows == Q.n_cols);
    assert(P.n_rows == Q.n_cols);
    assert(diver.n_elem == P.n_rows);

    arma::uword n = P.n_rows;
    for (arma::uword i = 0; i < n; i++)
        diver(i) = klDivergence(P.row(i), Q.row(i));
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
