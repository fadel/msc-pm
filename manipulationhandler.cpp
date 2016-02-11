#include "manipulationhandler.h"

#include <algorithm>

#include "mp.h"

ManipulationHandler::ManipulationHandler(const arma::mat &X,
                                         const arma::uvec &cpIndices,
                                         ProjectionHistory *history)
    : m_X(X)
    , m_cpIndices(cpIndices)
    , m_history(history)
    , m_technique(TECHNIQUE_LAMP)
{
    Q_ASSERT(history);
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

    emit mapChanged(Y);
}

void ManipulationHandler::setRewind(double t)
{
    if (!m_history->hasPrev()) {
        return;
    }

    arma::mat Y = m_history->Y() * t + m_history->prev() * (1.0 - t);
    emit mapRewound(Y);
}
