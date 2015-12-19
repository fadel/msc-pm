#ifndef INTERACTIONHANDLER_H
#define INTERACTIONHANDLER_H

#include <QObject>
#include <armadillo>

class InteractionHandler : public QObject
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

    InteractionHandler(const arma::mat &X, const arma::uvec &sampleIndices);
    void setTechnique(Technique technique);

signals:
    void subsampleChanged(const arma::mat &Y);

public slots:
    void setSubsample(const arma::mat &Ys);

protected:
    InteractionHandler() {}

private:
    arma::mat m_X, m_Y;
    arma::uvec m_sampleIndices;
    Technique m_technique;
};

#endif // INTERACTIONHANDLER_H
