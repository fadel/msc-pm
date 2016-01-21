#include "selectionhandler.h"

#include <QDebug>

SelectionHandler::SelectionHandler(int numItems)
    : m_selection(numItems, false)
{
}

void SelectionHandler::setSelection(const std::vector<bool> &selection)
{
    if (m_selection.size() != selection.size()) {
        return;
    }

    m_selection = selection;
    emit selectionChanged(m_selection);
}

void SelectionHandler::setSelected(int item, bool selected)
{
    m_selection[item] = selected;
    emit selectionChanged(m_selection);
}

void SelectionHandler::setSelected(const std::set<int> &items, bool selected)
{
    for (auto it = items.cbegin(); it != items.cend(); it++) {
        m_selection[*it] = selected;
    }

    emit selectionChanged(m_selection);
}
