#include "projectionhistory.h"

ProjectionHistory::ProjectionHistory(QObject *parent)
    : QObject(parent)
    , m_hasFirst(false)
    , m_hasPrev(false)
{
}

inline void ProjectionHistory::undo()
{
    if (m_hasPrev) {
        m_hasPrev = false;
        m_Y = m_prevY;

        emit historyChanged(m_Y);
    }
}

inline void ProjectionHistory::undoAll()
{
    if (m_hasFirst) {
        m_hasPrev = false;
        m_Y = m_firstY;

        emit historyChanged(m_Y);
    }
}

void ProjectionHistory::addMap(const arma::mat &Y)
{
    if (m_hasFirst) {
        m_hasPrev = true;
        m_prevY = m_Y;
    }

    m_Y = Y;

    if (!m_hasFirst) {
        m_hasFirst = true;
        m_firstY = m_Y;
    }

    emit historyChanged(m_Y);
}
