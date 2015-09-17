#include "effectivenessobserver.h"

EffectiveInteractionEnforcer::EffectiveInteractionEnforcer(const arma::uvec &sampleIndices)
    : m_sampleIndices(sampleIndices)
    , m_effectiveness(arma::zeros<arma::vec>(sampleIndices.n_elem))
{
}

void EffectiveInteractionEnforcer::setSelection(const arma::uvec &selection)
{
    m_selection = selection;
}

void EffectiveInteractionEnforcer::setMeasureDifference(const arma::vec &measure)
{
    m_measure = measure;

    if (m_selection.n_elem == 0) {
        return;
    }

    arma::uvec selectionIndices(m_selection);
    for (auto it = selectionIndices.begin(); it != selectionIndices.end(); it++) {
        *it = m_sampleIndices[*it];
    }

    double diff = arma::mean(m_measure(selectionIndices));
    int effectiveness;
    if (diff > 0) {
        effectiveness = 1;
    } else if (diff < 0) {
        effectiveness = -1;
    } else {
        effectiveness = 0;
    }

    for (auto it = m_selection.cbegin(); it != m_selection.cend(); it++) {
        m_effectiveness[*it] = effectiveness;
    }

    emit effectivenessChanged(m_effectiveness);
}
