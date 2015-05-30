#include <armadillo>

namespace mp {

double euclidean(const arma::rowvec &x1, const arma::rowvec &x2);
arma::mat dist(const arma::mat &X, double (*distCalc)(const arma::rowvec &, const arma::rowvec &) = euclidean);

arma::mat lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys);
void lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys, arma::mat &Y);

arma::mat forceScheme(const arma::mat &D, arma::mat &Y, size_t maxIter = 20, double tol = 1e-3, double fraction = 8);

arma::mat tSNE(const arma::mat &X, arma::uword k = 2, double perplexity = 30, arma::uword nIter = 1000);
void tSNE(const arma::mat &X, arma::mat &Y, double perplexity = 30, arma::uword nIter = 1000);

} // namespace mp
