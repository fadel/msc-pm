#include "mapscalehandler.h"

MapScaleHandler::MapScaleHandler()
    : m_sx(0.0f, 1.0f, 0.0f, 1.0f)
    , m_sy(0.0f, 1.0f, 0.0f, 1.0f)
{
}

void MapScaleHandler::scaleToMap(const arma::mat &Y)
{
    m_sx.setDomain(Y.col(0).min(), Y.col(0).max());
    m_sy.setDomain(Y.col(1).min(), Y.col(1).max());

    emit scaleChanged(m_sx, m_sy);
}
