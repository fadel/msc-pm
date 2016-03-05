#include "mp.h"

#include <algorithm>
#include <limits>

static const double EPSILON = 1e-3;

arma::mat mp::forceScheme(const arma::mat &D,
        arma::mat &Y,
        size_t maxIter,
        double tol,
        double fraction)
{
    arma::uword n = Y.n_rows;
    arma::uvec indices1(n), indices2(n);
    for (arma::uword k = 0; k < n; k++) {
        indices1[k] = indices2[k] = k;
    }

    double prevDeltaSum = std::numeric_limits<double>::infinity();
    for (size_t iter = 0; iter < maxIter; iter++) {
        double deltaSum = 0;

        arma::shuffle(indices1);
        for (auto i: indices1) {
            arma::shuffle(indices2);
            for (auto j: indices2) {
                if (i == j) {
                    continue;
                }

                arma::rowvec direction(Y.row(j) - Y.row(i));
                double d2 = std::max(arma::norm(direction, 2), EPSILON);
                double delta = (D(i, j) - d2) / fraction;
                deltaSum += fabs(delta);
                Y.row(j) += delta * (direction / d2);
            }
        }

        if (fabs(prevDeltaSum - deltaSum) < tol) {
            break;
        }
        prevDeltaSum = deltaSum;
    }

    return Y;
}
