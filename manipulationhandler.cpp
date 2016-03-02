#include "manipulationhandler.h"

#include <algorithm>

#include "mp.h"

ManipulationHandler::ManipulationHandler(const arma::mat &X,
                                         const arma::uvec &cpIndices)
    : m_X(X)
    , m_cpIndices(cpIndices)
    , m_technique(TECHNIQUE_LAMP)
{
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
