#include "interactionhandler.h"

#include "mp.h"

InteractionHandler::InteractionHandler(const arma::mat &X,
                                       const arma::uvec &cpIndices)
    : m_X(X)
    , m_Y(X.n_rows, 2)
    , m_cpIndices(cpIndices)
    , m_technique(TECHNIQUE_LAMP)
{
}

void InteractionHandler::setTechnique(InteractionHandler::Technique technique)
{
    if (m_technique == technique)
        return;

    m_technique = technique;
}

void InteractionHandler::setCP(const arma::mat &Ys)
{
    switch (m_technique) {
    case TECHNIQUE_PLMP:
        mp::plmp(m_X, m_cpIndices, Ys, m_Y);
        break;
    case TECHNIQUE_LSP:
        // TODO
        // mp::lsp(m_X, m_cpIndices, Ys, m_Y);
        break;
    case TECHNIQUE_LAMP:
        mp::lamp(m_X, m_cpIndices, Ys, m_Y);
        break;
    case TECHNIQUE_PEKALSKA:
        // TODO
        // mp::pekalska(m_X, m_cpIndices, Ys, m_Y);
        break;
    }

    emit cpChanged(m_Y);
}
