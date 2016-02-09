#include "manipulationhandler.h"

#include <algorithm>
#include <iostream>
#include <numeric>

#include <QDebug>

#include "mp.h"
#include "numericrange.h"

ManipulationHandler::ManipulationHandler(const arma::mat &X,
                                         const arma::uvec &cpIndices)
    : m_X(X)
    , m_Y(X.n_rows, 2)
    , m_cpIndices(cpIndices)
    , m_rpIndices(X.n_rows - cpIndices.n_elem)
    , m_technique(TECHNIQUE_LAMP)
{
    NumericRange<arma::uword> range(0, m_X.n_rows);
    std::set_symmetric_difference(range.cbegin(), range.cend(),
            m_cpIndices.cbegin(), m_cpIndices.cend(), m_rpIndices.begin());
}

void ManipulationHandler::setTechnique(ManipulationHandler::Technique technique)
{
    if (m_technique == technique)
        return;

    m_technique = technique;
}

void ManipulationHandler::setCP(const arma::mat &Ys)
{
    m_prevY = m_Y;

    switch (m_technique) {
    case TECHNIQUE_PLMP:
        // TODO?
        // mp::plmp(m_X, m_cpIndices, Ys, m_Y);
        break;
    case TECHNIQUE_LSP:
        // TODO?
        // mp::lsp(m_X, m_cpIndices, Ys, m_Y);
        break;
    case TECHNIQUE_LAMP:
        mp::lamp(m_X, m_cpIndices, Ys, m_Y);
        break;
    case TECHNIQUE_PEKALSKA:
        // TODO?
        // mp::pekalska(m_X, m_cpIndices, Ys, m_Y);
        break;
    }

    if (m_firstY.n_rows != m_Y.n_rows) {
        m_firstY = m_Y;
    }

    emit cpChanged(m_Y.rows(m_cpIndices));
    emit rpChanged(m_Y.rows(m_rpIndices));
    emit mapChanged(m_Y);
}

void ManipulationHandler::setRewind(double t)
{
    if (m_prevY.n_rows != m_Y.n_rows) {
        return;
    }

    arma::mat Y = m_Y * t + m_prevY * (1.0 - t);
    emit cpChanged(Y.rows(m_cpIndices));
    emit rpChanged(Y.rows(m_rpIndices));

    // NOTE: this signal was supposed to be emitted, but since we don't want
    // anything besides graphical objects to know the projection is being
    // rewound, this is (for now) left out.
    // emit mapChanged(Y);
}
