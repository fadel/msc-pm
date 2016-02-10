#ifndef MANIPULATIONHANDLER_H
#define MANIPULATIONHANDLER_H

#include <QObject>
#include <armadillo>

#include "projectionhistory.h"

class ManipulationHandler
    : public QObject
{
    Q_OBJECT
public:
    Q_ENUMS(Technique)
    enum Technique {
        TECHNIQUE_PLMP,
        TECHNIQUE_LAMP,
        TECHNIQUE_LSP,
        TECHNIQUE_PEKALSKA
    };

    ManipulationHandler(const arma::mat &X,
                        const arma::uvec &cpIndices,
                        ProjectionHistory *history);

    void setTechnique(Technique technique) { m_technique = technique; }

signals:
    void cpChanged(const arma::mat &cpY) const;
    void rpChanged(const arma::mat &rpY) const;
    void mapChanged(const arma::mat &Y) const;
    void cpRewound(const arma::mat &cpY) const;
    void rpRewound(const arma::mat &rpY) const;
    void mapRewound(const arma::mat &Y) const;

public slots:
    void setCP(const arma::mat &Ys);
    void setRewind(double t);

private:
    arma::mat m_X;
    arma::uvec m_cpIndices, m_rpIndices;
    ProjectionHistory *m_history;
    Technique m_technique;
};

#endif // MANIPULATIONHANDLER_H
