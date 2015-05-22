#ifndef INTERACTIONHANDLER_H
#define INTERACTIONHANDLER_H

#include <armadillo>

#include "scatterplot.h"

class InteractionHandler : public QObject
{
    Q_OBJECT
public:
    enum InteractiveTechnique {
        TECHNIQUE_PLMP,
        TECHNIQUE_LAMP,
        TECHNIQUE_LSP
    };

    InteractionHandler(const arma::mat &X, const arma::vec &labels, const arma::uvec &sampleIndices);

signals:
    void subsampleChanged(const arma::mat &Y);

public slots:
    void setSubsample(const arma::mat &Ys);

private:
    arma::mat m_X, m_Y;
    arma::vec m_labels;
    arma::uvec m_sampleIndices;
    InteractiveTechnique m_technique;
};

#endif // INTERACTIONHANDLER_H
