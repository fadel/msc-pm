#include "manipulationhandler.h"

#include <algorithm>

#include "mp.h"
#include "numericrange.h"

ManipulationHandler::ManipulationHandler(const arma::mat &X,
                                         const arma::uvec &cpIndices,
                                         ProjectionHistory *history)
    : m_X(X)
    , m_cpIndices(cpIndices)
    , m_rpIndices(X.n_rows - cpIndices.n_elem)
    , m_history(history)
    , m_technique(TECHNIQUE_LAMP)
{
    Q_ASSERT(history);

    NumericRange<arma::uword> allIndices(0, m_X.n_rows);
    std::set_symmetric_difference(allIndices.cbegin(), allIndices.cend(),
            m_cpIndices.cbegin(), m_cpIndices.cend(), m_rpIndices.begin());
}

void ManipulationHandler::setCP(const arma::mat &Ys)
{
    arma::mat Y(m_X.n_rows, 2);
    switch (m_technique) {
    case TECHNIQUE_PLMP:
        // TODO?
        break;
    case TECHNIQUE_LSP:
        // TODO?
        break;
    case TECHNIQUE_LAMP:
        mp::lamp(m_X, m_cpIndices, Ys, Y);
        break;
    case TECHNIQUE_PEKALSKA:
        // TODO?
        break;
    }

    emit cpChanged(Y.rows(m_cpIndices));
    emit rpChanged(Y.rows(m_rpIndices));
    emit mapChanged(Y);
}

void ManipulationHandler::setRewind(double t)
{
    if (!m_history->hasPrev()) {
        return;
    }

    arma::mat Y = m_history->Y() * t + m_history->prev() * (1.0 - t);
    emit cpRewound(Y.rows(m_cpIndices));
    emit rpRewound(Y.rows(m_rpIndices));
    emit mapRewound(Y);
}
