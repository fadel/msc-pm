#ifndef NPDISTORTION_H
#define NPDISTORTION_H

#include "distortionmeasure.h"

class NPDistortion : public DistortionMeasure
{
public:
    NPDistortion(int k = 10);
    arma::vec measure(const arma::mat &distA, const arma::mat &distB);

private:
    int m_k;
};

#endif // NPDISTORTION_H
