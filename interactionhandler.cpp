#include "interactionhandler.h"

#include "mp.h"

InteractionHandler::InteractionHandler(const arma::mat &X,
                                       const arma::vec &labels,
                                       const arma::uvec &sampleIndices)
    : m_X(X)
    , m_Y(X.n_rows, 3)
    , m_labels(labels)
    , m_sampleIndices(sampleIndices)
    , m_Y(X.n_rows, 3)
    , m_technique(TECHNIQUE_LAMP)
{
    m_Y.col(2) = m_labels;
}

void InteractionHandler::setSubsample(const arma::mat &Ys)
{
    switch (m_technique) {
    case TECHNIQUE_PLMP:
    case TECHNIQUE_LSP:
    case TECHNIQUE_LAMP:
        mp::lamp(m_X, m_sampleIndices, Ys, m_Y);
        break;
    }

    emit subsampleChanged(m_Y);
}
