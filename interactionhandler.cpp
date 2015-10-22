#include "interactionhandler.h"

#include "mp.h"

InteractionHandler::InteractionHandler(const arma::mat &X,
                                       const arma::uvec &sampleIndices)
    : m_X(X)
    , m_Y(X.n_rows, 2)
    , m_sampleIndices(sampleIndices)
    , m_technique(TECHNIQUE_LAMP)
{
}

void InteractionHandler::setSubsample(const arma::mat &Ys)
{
    switch (m_technique) {
    case TECHNIQUE_PLMP:
        mp::plmp(m_X, m_sampleIndices, Ys, m_Y);
        break;
    case TECHNIQUE_LSP:
        // mp::lsp(m_X, m_sampleIndices, Ys, m_Y);
        break;
    case TECHNIQUE_LAMP:
        mp::lamp(m_X, m_sampleIndices, Ys, m_Y);
        break;
    }

    emit subsampleChanged(m_Y);
}
