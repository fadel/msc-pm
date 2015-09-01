#ifndef DISTORTIONMEASURE_H
#define DISTORTIONMEASURE_H

#include <armadillo>

class DistortionMeasure
{
public:
    virtual arma::vec measure(const arma::mat &distA, const arma::mat &distB) = 0;
};

#endif // DISTORTIONMEASURE_H
