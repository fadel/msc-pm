#ifndef INTERACTIONHANDLER_H
#define INTERACTIONHANDLER_H

#include <QObject>
#include <armadillo>

class InteractionHandler : public QObject
{
    Q_OBJECT
public:
    enum InteractiveTechnique {
        TECHNIQUE_PLMP,
        TECHNIQUE_LAMP,
        TECHNIQUE_LSP
    };

    InteractionHandler(const arma::mat &X, const arma::uvec &sampleIndices);

signals:
    void subsampleChanged(const arma::mat &Y);

public slots:
    void setSubsample(const arma::mat &Ys);

private:
    arma::mat m_X, m_Y;
    arma::uvec m_sampleIndices;
    InteractiveTechnique m_technique;
};

#endif // INTERACTIONHANDLER_H
