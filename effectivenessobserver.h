#ifndef EFFECTIVEINTERACTIONENFORCER_H
#define EFFECTIVEINTERACTIONENFORCER_H

#include <QObject>
#include <armadillo>

class EffectiveInteractionEnforcer : public QObject
{
    Q_OBJECT
public:
    EffectiveInteractionEnforcer(const arma::uvec &sampleIndices);

signals:
    void effectivenessChanged(const arma::vec &effectiveness);

public slots:
    void setSelection(const arma::uvec &selection);
    void setMeasureDifference(const arma::vec &measure);

private:
    arma::mat m_effectiveness;
    arma::uvec m_sampleIndices, m_selection;
    arma::vec m_measure;
};

#endif // EFFECTIVEINTERACTIONENFORCER_H
