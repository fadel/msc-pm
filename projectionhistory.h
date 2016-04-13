#ifndef PROJECTIONHISTORY_H
#define PROJECTIONHISTORY_H

#include <vector>

#include <QObject>

#include <armadillo>

class ProjectionHistory
    : public QObject
{
    Q_OBJECT
public:
    enum ObserverType {
        ObserverCurrent,
        ObserverDiffPrevious,
        ObserverDiffFirst
    };

    ProjectionHistory(const arma::mat &X, const arma::uvec &cpIndices);

    const arma::mat &Y() const             { return m_Y; }
    const arma::mat &firstY() const        { return m_firstY; }
    const arma::mat &prevY() const         { return m_prevY; }
    const arma::mat &unreliability() const { return m_unreliability; }

    const arma::uvec &cpIndices() const { return m_cpIndices; }
    const arma::uvec &rpIndices() const { return m_rpIndices; }

    bool hasFirst() const { return m_hasFirst; }
    bool hasPrev() const  { return m_hasPrev; }

    void undo();
    void reset();

signals:
    void undoPerformed() const;
    void resetPerformed() const;

    void currentMapChanged(const arma::mat &Y) const;
    void valuesChanged(const arma::vec &values, bool rescale) const;
    void cpValuesChanged(const arma::vec &values, bool rescale) const;
    void rpValuesChanged(const arma::vec &values, bool rescale) const;

    void mapRewound(const arma::mat &Y) const;
    void valuesRewound(const arma::vec &values) const;
    void cpValuesRewound(const arma::vec &values) const;
    void rpValuesRewound(const arma::vec &values) const;

    void cpSelectionChanged(const std::vector<bool> &cpSelection) const;
    void rpSelectionChanged(const std::vector<bool> &rpSelection) const;
    void selectionChanged(const std::vector<bool> &selection) const;

public slots:
    void addMap(const arma::mat &Y);

    bool setType(ObserverType type);
    void setCPSelection(const std::vector<bool> &cpSelection);
    void setRPSelection(const std::vector<bool> &rpSelection);
    void setSelection(const std::vector<bool> &selection);

    void setRewind(double t);

private:
    bool emitValuesChanged() const;
    void updateUnreliability();

    void cpSelectionPostProcess();
    void rpSelectionPostProcess();

    ObserverType m_type;

    arma::mat m_X, m_Y, m_firstY, m_prevY;
    arma::mat m_distX, m_distY, m_firstDistY, m_prevDistY;
    arma::mat m_unreliability;
    arma::uvec m_cpIndices, m_rpIndices;

    bool m_cpSelectionEmpty, m_rpSelectionEmpty;
    std::vector<int> m_cpSelection, m_rpSelection;
    std::vector<bool> m_selection;

    // alpha(i, j): the influence CP j has on RP i
    void computeAlphas();
    arma::mat m_alphas, m_influences;

    // TODO: one per implemented measure
    arma::vec m_values, m_firstValues, m_prevValues;

    bool m_hasFirst, m_hasPrev;
};

#endif // PROJECTIONHISTORY_H
