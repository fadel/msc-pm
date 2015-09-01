#include "distortionobserver.h"

#include "mp.h"

DistortionObserver::DistortionObserver(const arma::mat &X,
                                       const arma::uvec &sampleIndices)
    : m_X(X)
    , m_sampleIndices(sampleIndices)
    , m_distortionMeasure(0)
{
    m_distX = mp::dist(m_X);
}

void DistortionObserver::setMeasure(DistortionMeasure *measure)
{
    m_distortionMeasure = measure;
}

void DistortionObserver::setMap(const arma::mat &Y)
{
    if (!m_distortionMeasure) {
        return;
    }

    arma::vec measures = m_distortionMeasure->measure(m_distX, mp::dist(Y));

    if (m_Y.n_elem != 0) {
        emit mapChanged(measures - m_measures);
    } else {
        m_Y = Y;
        m_measures = measures;
    }
}
