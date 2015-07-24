#include "distortionobserver.h"

#include "mp.h"

DistortionObserver::DistortionObserver(const arma::mat &X,
                                       const arma::uvec &sampleIndices)
    : m_X(X)
    , m_sampleIndices(sampleIndices)
{
    m_distX = mp::dist(m_X);
}

DistortionObserver::~DistortionObserver()
{
}

void DistortionObserver::setMap(const arma::mat &Y)
{
    arma::vec measures = measureFunc(m_distX, mp::dist(Y));

    if (m_Y.n_elem != 0) {
        emit mapChanged(measures - m_measures);
    } else {
        m_Y = Y;
        m_measures = measures;
    }
}
