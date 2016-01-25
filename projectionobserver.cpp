#include "projectionobserver.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "mp.h"
#include "numericrange.h"

static const float EPSILON = 1e-6f;

static void aggregatedError(const arma::mat &distX, const arma::mat &distY, arma::vec &v)
{
    double maxX = distX.max();
    double maxY = distY.max();

    #pragma omp parallel for shared(maxX, maxY, distX, distY, v)
    for (arma::uword i = 0; i < v.n_elem; i++) {
        v[i] = 0;
        for (arma::uword j = 0; j < v.n_elem; j++) {
            if (i == j) {
                continue;
            }

            float diff = fabs(distY(i, j) / maxY - distX(i, j) / maxX);
            if (diff < EPSILON) {
                continue;
            }

            v[i] += diff;
        }
    }
}

ProjectionObserver::ProjectionObserver(const arma::mat &X,
                                       const arma::uvec &cpIndices)
    : m_type(OBSERVER_CURRENT)
    , m_X(X)
    , m_cpIndices(cpIndices)
    , m_rpIndices(X.n_rows - cpIndices.n_elem)
{
    m_distX = mp::dist(m_X);
    m_values.set_size(m_X.n_rows);

    NumericRange<unsigned long long> range(0, m_X.n_rows);
    std::set_symmetric_difference(range.cbegin(), range.cend(),
            m_cpIndices.cbegin(), m_cpIndices.cend(), m_rpIndices.begin());
}

bool ProjectionObserver::setType(int type)
{
    if (m_type == type) {
        return true;
    }

    if (type != OBSERVER_DIFF_PREVIOUS || m_prevValues.n_elem != 0) {
        m_type = type;
        return emitValuesChanged();
    }

    return false;
}

void ProjectionObserver::setMap(const arma::mat &Y)
{
    // update previous map
    if (m_Y.n_elem > 0) {
        m_prevY = m_Y;
        m_prevDistY = m_distY;
        m_prevValues = m_values;
    }

    m_Y = Y;
    m_distY = mp::dist(Y);
    aggregatedError(m_distX, m_distY, m_values);

    // method called for the first time; set original Y
    if (m_origY.n_elem == 0) {
        m_origY = m_Y;
        m_origDistY = m_distY;
        m_origValues = m_values;
    }

    emitValuesChanged();
}

bool ProjectionObserver::emitValuesChanged() const
{
    switch (m_type) {
    case OBSERVER_CURRENT:
        emit cpValuesChanged(m_values(m_cpIndices));
        emit rpValuesChanged(m_values(m_rpIndices));
        emit valuesChanged(m_values);
        return true;
    case OBSERVER_DIFF_PREVIOUS:
        if (m_prevValues.n_elem > 0) {
            arma::vec diff = m_values - m_prevValues;
            emit cpValuesChanged(diff(m_cpIndices));
            emit rpValuesChanged(diff(m_rpIndices));
            emit valuesChanged(diff);
            return true;
        }
        return false;
    case OBSERVER_DIFF_ORIGINAL:
        if (m_origValues.n_elem > 0) {
            arma::vec diff = m_values - m_origValues;
            emit cpValuesChanged(diff(m_cpIndices));
            emit rpValuesChanged(diff(m_rpIndices));
            emit valuesChanged(diff);
            return true;
        }
        return false;
    default:
        return false;
    }
}
