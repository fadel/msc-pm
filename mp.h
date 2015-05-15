#include <armadillo>

namespace mp {

static const double EPSILON = 1e-3;
double euclidean(const arma::rowvec &x1, const arma::rowvec &x2);
arma::mat dist(const arma::mat &X, double (*distCalc)(const arma::rowvec &, const arma::rowvec &) = euclidean);
arma::mat lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys);
arma::mat forceScheme(const arma::mat &D, arma::mat &Y, size_t maxIter = 20, double tol = 1e-3, double fraction = 0.25);

} // namespace mp
