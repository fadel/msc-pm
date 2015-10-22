
#include "mp.h"

arma::mat mp::plmp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys)
{
    arma::mat Y(X.n_rows, Ys.n_cols);
    mp::plmp(X, sampleIndices, Ys, Y);
    return Y;
}

void mp::plmp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys, arma::mat &Y)
{
    arma::mat Xs = X.rows(sampleIndices);
    Xs.each_row() -= arma::mean(Xs);
    arma::mat lYs = Ys;
    lYs.each_row() -= arma::mean(Ys);
    const arma::mat &Xst = Xs.t();
    arma::mat P = arma::solve(Xst * Xs, Xst * lYs);

    Y = X * P;
    Y.rows(sampleIndices) = lYs;
}
