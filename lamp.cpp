#include "mp.h"

#include <algorithm>

#include "utils.h"

static const double EPSILON = 1e-6;

arma::mat mp::lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys)
{
    arma::mat projection(X.n_rows, 2);
    lamp(X, sampleIndices, Ys, projection);
    return projection;
}

void mp::lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys, arma::mat &Y)
{
    int n = uintToInt<arma::uword, int>(X.n_rows);
    const arma::mat &Xs = X.rows(sampleIndices);
    arma::uword sampleSize = sampleIndices.n_elem;

    #pragma omp parallel for shared(X, Xs, Ys, Y, n)
    for (int i = 0; i < n; i++) {
        const arma::rowvec &point = X.row(i);

        // calculate alphas
        arma::rowvec alphas(sampleSize);
        for (arma::uword j = 0; j < sampleSize; j++) {
            double dist = arma::accu(arma::square(Xs.row(j) - point));
            alphas[j] = 1. / std::max(dist, EPSILON);
        }

        double alphas_sum = arma::accu(alphas);

        // calculate \tilde{X} and \tilde{Y}
        arma::rowvec Xtil = arma::sum(alphas * Xs, 0) / alphas_sum;
        arma::rowvec Ytil = arma::sum(alphas * Ys, 0) / alphas_sum;

        // calculate \hat{X} and \hat{Y}
        arma::mat Xhat = Xs;
        Xhat.each_row() -= Xtil;
        arma::mat Yhat = Ys;
        Yhat.each_row() -= Ytil;

        // calculate A and B
        alphas = arma::sqrt(alphas);
        arma::mat &At = Xhat;
        inplace_trans(At);
        At.each_row() %= alphas;
        arma::mat &B = Yhat;
        B.each_col() %= alphas.t();

        arma::mat U, V;
        arma::vec s(Ys.n_cols);
        arma::svd(U, s, V, At * B);
        arma::mat M = U.head_cols(Ys.n_cols) * V.t();

        Y.row(i) = (point - Xtil) * M + Ytil;
    }

    for (arma::uword i = 0; i < sampleSize; i++) {
        Y.row(sampleIndices[i]) = Ys.row(i);
    }
}
