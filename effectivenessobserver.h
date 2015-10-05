#ifndef EFFECTIVEINTERACTIONENFORCER_H
#define EFFECTIVEINTERACTIONENFORCER_H

#include <QObject>
#include <QSet>
#include <armadillo>

class EffectiveInteractionEnforcer : public QObject
{
    Q_OBJECT
public:
    EffectiveInteractionEnforcer(const arma::uvec &sampleIndices);

signals:
    void effectivenessChanged(const arma::vec &effectiveness);

public slots:
    void setSelection(const QSet<int> &selection);
    void setMeasureDifference(const arma::vec &measure);

private:
    arma::uvec m_sampleIndices;
    arma::mat m_effectiveness;
    arma::vec m_measure;
    QSet<int> m_selection;
};

#endif // EFFECTIVEINTERACTIONENFORCER_H
