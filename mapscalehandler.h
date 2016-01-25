#ifndef MAPSCALEHANDLER_H
#define MAPSCALEHANDLER_H

#include <QObject>
#include <armadillo>

#include "scale.h"

class MapScaleHandler
    : public QObject
{
    Q_OBJECT
public:
    MapScaleHandler();

    void getScales(LinearScale<float> &sx, LinearScale<float> &sy) const {
        sx = m_sx;
        sy = m_sy;
    }

signals:
    void scaleChanged(const LinearScale<float> &sx,
                      const LinearScale<float> &sy) const;

public slots:
    void scaleToMap(const arma::mat &Y);

private:
    LinearScale<float> m_sx, m_sy;
};

#endif // MAPSCALEHANDLER_H
