#include "mp.h"

#include <algorithm>

static const double EPSILON = 1e-3;

arma::mat mp::lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys)
{
    arma::mat projection(X.n_rows, 2);
    lamp(X, sampleIndices, Ys, projection);
    return projection;
}

void mp::lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys, arma::mat &Y)
{
    const arma::mat &Xs = X.rows(sampleIndices);
    arma::uword sampleSize = sampleIndices.n_elem;

    #pragma omp parallel for shared(X, Xs, Ys, Y)
    for (arma::uword i = 0; i < X.n_rows; i++) {
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

        arma::mat U(Ys.n_rows, Ys.n_cols), V(Ys.n_cols, Ys.n_cols);
        arma::vec s(Ys.n_cols);
        arma::svd(U, s, V, At * B);
        arma::mat M = U.cols(0, 1) * V.t();

        Y(i, arma::span(0, 1)) = (point - Xtil) * M + Ytil;
    }

    for (arma::uword i = 0; i < sampleSize; i++) {
        Y.row(sampleIndices[i]) = Ys.row(i);
    }
}
