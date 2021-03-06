#ifndef MP_H
#define MP_H

#include <armadillo>

namespace mp {

// Distance-related
typedef double (*DistFunc)(const arma::rowvec &, const arma::rowvec &);
double euclidean(const arma::rowvec &x1, const arma::rowvec &x2);
arma::mat dist(const arma::mat &X, DistFunc dfunc = euclidean);

void knn(const arma::mat &dmat, arma::uword i, arma::uword k, arma::uvec &nn, arma::vec &dist);

// Evaluation measures
void neighborhoodPreservation(const arma::mat &distA, const arma::mat &distB, arma::uword k, arma::vec &v);
arma::vec silhouette(const arma::mat &distA, const arma::mat &distB, const arma::vec &labels);
void aggregatedError(const arma::mat &distX, const arma::mat &distY, arma::vec &v);

// Techniques
arma::mat lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys);
void lamp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys, arma::mat &Y);

arma::mat plmp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys);
void plmp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys, arma::mat &Y);

arma::mat pekalska(const arma::mat &D, const arma::uvec &sampleIndices, const arma::mat &Ys);
void pekalska(const arma::mat &D, const arma::uvec &sampleIndices, const arma::mat &Ys, arma::mat &Y);

//arma::mat lsp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys, int k = 15);
//void lsp(const arma::mat &X, const arma::uvec &sampleIndices, const arma::mat &Ys, arma::mat &Y, int k = 15);

arma::mat forceScheme(const arma::mat &D, arma::mat &Y, size_t maxIter = 20, double tol = 1e-3, double fraction = 8);

arma::mat tSNE(const arma::mat &X, arma::uword k = 2, double perplexity = 30, arma::uword nIter = 1000);
void tSNE(const arma::mat &X, arma::mat &Y, double perplexity = 30, arma::uword nIter = 1000);

} // namespace mp

#endif // MP_H
