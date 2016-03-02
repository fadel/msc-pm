#include "projectionhistory.h"

#include <algorithm>
#include <cmath>

#include <QDebug>

#include "mp.h"
#include "numericrange.h"

ProjectionHistory::ProjectionHistory(const arma::mat &X,
                                     const arma::uvec &cpIndices)
    : m_type(ObserverCurrent)
    , m_X(X)
    , m_cpIndices(cpIndices)
    , m_rpIndices(X.n_rows - cpIndices.n_elem)
    , m_cpSelectionEmpty(true)
    , m_rpSelectionEmpty(true)
    , m_values(X.n_rows)
    , m_prevValues(X.n_rows)
    , m_firstValues(X.n_rows)
    , m_hasFirst(false)
    , m_hasPrev(false)
{
    m_distX = mp::dist(m_X);

    NumericRange<arma::uword> allIndices(0, m_X.n_rows);
    std::set_symmetric_difference(allIndices.cbegin(), allIndices.cend(),
            m_cpIndices.cbegin(), m_cpIndices.cend(), m_rpIndices.begin());

    computeAlphas();
}

void ProjectionHistory::computeAlphas()
{
    m_influences.set_size(m_X.n_rows);
    m_alphas.set_size(m_rpIndices.n_elem, m_cpIndices.n_elem);

    for (arma::uword i = 0; i < m_rpIndices.n_elem; i++) {
        double sum = 0;
        const arma::rowvec &x = m_X.row(m_rpIndices[i]);
        for (arma::uword j = 0; j < m_cpIndices.n_elem; j++) {
            double norm = arma::norm(x - m_X.row(m_cpIndices[j]));
            m_alphas(i, j) = 1.0 / std::max(norm * norm, 1e-6);
            sum += m_alphas(i, j);
        }

        for (arma::uword j = 0; j < m_cpIndices.n_elem; j++) {
            m_alphas(i, j) /= sum;
        }
    }
}
void ProjectionHistory::undo()
{
    if (m_hasPrev) {
        m_hasPrev = false;
        m_Y = m_prevY;
        m_distY  = m_prevDistY;
        m_values = m_prevValues;

        emit undoPerformed();
        emit currentMapChanged(m_Y);
        if (m_cpSelectionEmpty && m_rpSelectionEmpty) {
            emitValuesChanged();
        }
    }
}

void ProjectionHistory::reset()
{
    if (m_hasFirst) {
        m_hasPrev = false;
        m_Y = m_firstY;
        m_distY  = m_firstDistY;
        m_values = m_firstValues;

        emit resetPerformed();
        emit currentMapChanged(m_Y);
        if (m_cpSelectionEmpty && m_rpSelectionEmpty) {
            emitValuesChanged();
        }
    }
}

void ProjectionHistory::addMap(const arma::mat &Y)
{
    if (m_hasFirst) {
        m_hasPrev = true;
        m_prevY = m_Y;
        m_prevDistY = m_distY;
        m_prevValues = m_values;
    }

    m_Y = Y;
    m_distY = mp::dist(Y);

    mp::aggregatedError(m_distX, m_distY, m_values);
    qDebug("Aggr. error: min: %f, max: %f", m_values.min(), m_values.max());

    if (!m_hasFirst) {
        m_hasFirst = true;
        m_firstY = m_Y;
        m_firstDistY = m_distY;
        m_firstValues = m_values;
    }

    emit currentMapChanged(m_Y);
    if (m_cpSelectionEmpty && m_rpSelectionEmpty) {
        emitValuesChanged();
    }
}

bool ProjectionHistory::setType(ObserverType type)
{
    if (m_type == type) {
        return true;
    }

    if ((type == ObserverDiffPrevious && !m_hasPrev)
        || (type == ObserverDiffFirst && !m_hasFirst)) {
        return false;
    }

    m_type = type;
    if (!m_cpSelectionEmpty || !m_rpSelectionEmpty) {
        // We changed our type, but cannot emit values since we have non-empty
        // selections
        return true;
    }
    return emitValuesChanged();
}

void ProjectionHistory::setCPSelection(const std::vector<bool> &cpSelection)
{
    m_cpSelection.clear();
    for (int i = 0; i < cpSelection.size(); i++) {
        if (cpSelection[i]) {
            m_cpSelection.push_back(i);
        }
    }
    m_cpSelectionEmpty = m_cpSelection.empty();

    if (!m_cpSelectionEmpty) {
        // compute the influence of CP selection on each RP
        for (arma::uword rp = 0; rp < m_rpIndices.n_elem; rp++) {
            m_influences[m_rpIndices[rp]] = 0;
            const arma::rowvec &row = m_alphas.row(rp);
            for (auto cp: m_cpSelection) {
                m_influences[m_rpIndices[rp]] += row[cp];
            }
        }

        emit rpValuesChanged(m_influences(m_rpIndices), true);
    } else {
        emitValuesChanged();
    }
}

void ProjectionHistory::setRPSelection(const std::vector<bool> &rpSelection)
{
    m_rpSelection.clear();
    for (int i = 0; i < rpSelection.size(); i++) {
        if (rpSelection[i]) {
            m_rpSelection.push_back(i);
        }
    }
    m_rpSelectionEmpty = m_rpSelection.empty();

    if (!m_rpSelectionEmpty) {
        // compute how influent is each CP on RP selection
        for (arma::uword cp = 0; cp < m_cpIndices.n_elem; cp++) {
            m_influences[m_cpIndices[cp]] = 0;
            for (auto rp: m_rpSelection) {
                m_influences[m_cpIndices[cp]] += m_alphas(rp, cp);
            }
        }

        emit cpValuesChanged(m_influences(m_cpIndices), true);
    } else {
        emit cpValuesChanged(arma::vec(), false);
    }
}

bool ProjectionHistory::emitValuesChanged() const
{
    switch (m_type) {
    case ObserverCurrent:
        emit rpValuesChanged(m_values(m_rpIndices), false);
        emit valuesChanged(m_values, false);
        return true;
    case ObserverDiffPrevious:
        if (m_hasPrev) {
            arma::vec diff = m_values - m_prevValues;
            emit rpValuesChanged(diff(m_rpIndices), true);
            emit valuesChanged(diff, false);
            return true;
        }
        return false;
    case ObserverDiffFirst:
        if (m_hasFirst) {
            arma::vec diff = m_values - m_firstValues;
            emit rpValuesChanged(diff(m_rpIndices), true);
            emit valuesChanged(diff, true);
            return true;
        }
        return false;
    default:
        return false;
    }
}

void ProjectionHistory::setRewind(double t)
{
    if (!m_hasPrev) {
        return;
    }

    arma::mat Y = m_Y * t + m_prevY * (1.0 - t);
    emit mapRewound(Y);

    if (!m_cpSelectionEmpty || !m_rpSelectionEmpty) {
        return;
    }

    arma::vec values = m_values * t + m_prevValues * (1.0 - t);
    // emit cpValuesRewound(values(m_cpIndices));
    emit rpValuesRewound(values(m_rpIndices));
    emit valuesRewound(values);
}
