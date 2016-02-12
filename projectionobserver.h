#ifndef PROJECTIONOBSERVER_H
#define PROJECTIONOBSERVER_H

#include <QObject>
#include <armadillo>

#include "projectionhistory.h"

class ProjectionObserver
    : public QObject
{
    Q_OBJECT
public:
    enum ObserverType {
        ObserverCurrent,
        ObserverDiffPrevious,
        ObserverDiffFirst
    };

    ProjectionObserver(const arma::mat &X,
                       const arma::uvec &cpIndices,
                       ProjectionHistory *history);

signals:
    void valuesChanged(const arma::vec &values) const;
    void cpValuesChanged(const arma::vec &values) const;
    void rpValuesChanged(const arma::vec &values) const;
    void valuesRewound(const arma::vec &values) const;
    void cpValuesRewound(const arma::vec &values) const;
    void rpValuesRewound(const arma::vec &values) const;

public slots:
    void setMap(const arma::mat &Y);
    bool setType(int type);
    void setCPSelection(const std::vector<bool> &cpSelection);
    void setRPSelection(const std::vector<bool> &rpSelection);
    void setRewind(double t);

private:
    bool emitValuesChanged() const;

    int m_type;
    arma::mat m_X;
    arma::mat m_distX, m_distY, m_firstDistY, m_prevDistY;
    arma::uvec m_cpIndices, m_rpIndices;

    bool m_cpSelectionEmpty, m_rpSelectionEmpty;
    std::vector<int> m_cpSelection, m_rpSelection;

    // alpha(i, j): the influence CP j has on RP i
    void computeAlphas();
    arma::mat m_alphas, m_influences;

    // TODO: one per implemented measure
    arma::vec m_values, m_firstValues, m_prevValues;
    ProjectionHistory *m_history;
};

#endif // PROJECTIONOBSERVER_H
