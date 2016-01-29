#include "mp.h"

#include <algorithm>

static const double EPSILON = 1e-3;

typedef arma::uvec V;

arma::mat mp::forceScheme(const arma::mat &D,
        arma::mat &Y,
        size_t maxIter,
        double tol,
        double fraction)
{
    arma::uword n = Y.n_rows;
    V i(n), j(n);
    for (arma::uword k = 0; k < n; k++) {
        i[k] = j[k] = k;
    }

    double prevDeltaSum = 1. / 0.;
    for (size_t iter = 0; iter < maxIter; iter++) {
        double deltaSum = 0;

        arma::shuffle(i);
        for (V::iterator a = i.begin(); a != i.end(); a++) {
            arma::shuffle(j);
            for (V::iterator b = j.begin(); b != j.end(); b++) {
                if (*a == *b) {
                    continue;
                }

                arma::rowvec direction(Y.row(*b) - Y.row(*a));
                double d2 = std::max(arma::norm(direction, 2), EPSILON);
                double delta = (D(*a, *b) - d2) / fraction;
                deltaSum += fabs(delta);
                Y.row(*b) += delta * (direction / d2);
            }
        }

        if (fabs(prevDeltaSum - deltaSum) < tol) {
            break;
        }
        prevDeltaSum = deltaSum;
    }

    return Y;
}
