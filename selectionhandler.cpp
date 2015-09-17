#include "selectionhandler.h"

SelectionHandler::SelectionHandler(const arma::uvec &sampleIndices)
    : m_sampleIndices(sampleIndices)
{
}

void SelectionHandler::setSelection(const arma::uvec &selection)
{
    arma::uvec newSelection(selection);

    // The selecion happens over the sample indices. We use the original dataset
    // indices in sampleIndices to translate indices.
    for (auto it = newSelection.begin(); it != newSelection.end(); it++) {
        *it = m_sampleIndices[*it];
    }

    emit selectionChanged(newSelection);
}
