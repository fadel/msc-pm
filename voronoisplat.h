#ifndef VORONOISPLAT_H
#define VORONOISPLAT_H

#include <QQuickFramebufferObject>

#include <armadillo>

#include "colorscale.h"
#include "scale.h"

class VoronoiSplat
    : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(float alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)
    Q_PROPERTY(float beta  READ beta  WRITE setBeta  NOTIFY betaChanged)
public:
    VoronoiSplat(QQuickItem *parent = 0);

    Renderer *createRenderer() const;

    const std::vector<float> &sites() const      { return m_sites; }
    const std::vector<float> &values() const     { return m_values; }
    const std::vector<float> &colorScale() const { return m_cmap; }
    LinearScale<float> scaleX() const { return m_sx; }
    LinearScale<float> scaleY() const { return m_sy; }
    float alpha() const { return m_alpha; }
    float beta() const  { return m_beta; }

    bool sitesChanged() const      { return m_sitesChanged; }
    bool valuesChanged() const     { return m_valuesChanged; }
    bool colormapChanged() const { return m_colormapChanged; }

    void setSitesChanged(bool sitesChanged) {
        m_sitesChanged = sitesChanged;
    }
    void setValuesChanged(bool valuesChanged) {
        m_valuesChanged = valuesChanged;
    }
    void setColormapChanged(bool colormapChanged) {
        m_colormapChanged = colormapChanged;
    }

signals:
    void sitesChanged(const arma::mat &sites) const;
    void valuesChanged(const arma::vec &values) const;
    void colorScaleChanged(const ColorScale &scale) const;
    void scaleChanged(const LinearScale<float> &sx, const LinearScale<float> &sy) const;
    void alphaChanged(float alpha) const;
    void betaChanged(float alpha) const;

public slots:
    // 'points' should be a 2D points matrix (each point in a row)
    void setSites(const arma::mat &points);

    // Set the value to be colorScaleped in each site
    void setValues(const arma::vec &values);

    // Set colorScale data based on the given color scale
    void setColorScale(const ColorScale &scale);

    void setScale(const LinearScale<float> &sx, const LinearScale<float> &sy);

    // Shepard blur radius
    void setAlpha(float alpha);

    // Maximum blur radius
    void setBeta(float beta);

private:
    std::vector<float> m_sites, m_values, m_cmap;
    LinearScale<float> m_sx, m_sy;
    float m_alpha, m_beta;
    bool m_sitesChanged, m_valuesChanged, m_colormapChanged;
};

#endif // VORONOISPLAT_H
