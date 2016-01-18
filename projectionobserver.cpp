#include "projectionobserver.h"

#include <cmath>

#include "mp.h"

static void aggregatedError(const arma::mat &distX, const arma::mat &distY, arma::vec &v)
{
    double maxX = distX.max();
    double maxY = distY.max();

    for (arma::uword i = 0; i < v.n_elem; i++) {
        v[i] = 0;
        for (arma::uword j = 0; j < v.n_elem; j++) {
            if (i == j) {
                continue;
            }

            v[i] += fabs(distY(i, j) / maxY - distX(i, j) / maxX);
        }
    }
}

ProjectionObserver::ProjectionObserver(const arma::mat &X,
                                       const arma::uvec &cpIndices)
    : m_X(X)
    , m_cpIndices(cpIndices)
{
    m_distX = mp::dist(m_X);
    m_values.set_size(m_X.n_rows);
}

void ProjectionObserver::setMap(const arma::mat &Y)
{
    // update previous map
    if (m_prevY.n_elem == 0 && m_Y.n_elem > 0) {
        m_prevY = m_Y;
        m_distPrevY = m_distY;
    }

    m_Y = Y;
    m_distY = mp::dist(Y);

    // method called for the first time; set original Y
    if (m_origY.n_elem == 0) {
        m_origY = m_Y;
        m_distOrigY = m_distY;
    }

    aggregatedError(m_distX, m_distY, m_values);
    emit mapChanged(m_values);
}
