#include "selectionhandler.h"

SelectionHandler::SelectionHandler(const arma::uvec &sampleIndices)
    : m_sampleIndices(sampleIndices)
{
}

void SelectionHandler::setSelection(const QSet<int> &selection)
{
    QSet<int> newSelection;

    // The selecion happens over the sample indices. We use the original dataset
    // indices in sampleIndices to translate indices.
    for (auto it = selection.begin(); it != selection.end(); it++) {
        newSelection.insert(m_sampleIndices[*it]);
    }

    emit selectionChanged(newSelection);
}
