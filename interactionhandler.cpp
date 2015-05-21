#include "interactionhandler.h"

#include "mp.h"

InteractionHandler::InteractionHandler(const arma::mat &X,
                                       const arma::vec &labels,
                                       const arma::uvec &sampleIndices)
    : m_X(X)
    , m_labels(labels)
    , m_sampleIndices(sampleIndices)
    , m_technique(TECHNIQUE_LAMP)
{
}

void InteractionHandler::setSubsample(const arma::mat &Ys)
{
    arma::mat embedding(m_X.n_rows, Ys.n_cols);
    switch (m_technique) {
    case TECHNIQUE_PLMP:
    case TECHNIQUE_LSP:
    case TECHNIQUE_LAMP:
        embedding = mp::lamp(m_X, m_sampleIndices, Ys);
        break;
    }

    arma::mat Y(embedding.n_rows, embedding.n_cols + 1);
    Y.cols(0, embedding.n_cols - 1) = embedding;
    Y.col(Y.n_cols - 1) = m_labels;

    emit subsampleChanged(Y);
}
