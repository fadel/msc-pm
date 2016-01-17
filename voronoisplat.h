#ifndef VORONOISPLAT_H
#define VORONOISPLAT_H

#include <QQuickFramebufferObject>

#include <armadillo>

#include "colorscale.h"

class VoronoiSplat
    : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    VoronoiSplat(QQuickItem *parent = 0);

    Renderer *createRenderer() const;

    const std::vector<float> &sites() const      { return m_sites; }
    const std::vector<float> &values() const     { return m_values; }
    const std::vector<float> &colorScale() const { return m_cmap; }
    Q_INVOKABLE float alpha() const { return m_alpha; }
    Q_INVOKABLE float beta() const  { return m_beta; }

    bool sitesChanged() const      { return m_sitesChanged; }
    bool valuesChanged() const     { return m_valuesChanged; }
    bool colorScaleChanged() const { return m_colorScaleChanged; }

    void setSitesChanged(bool sitesChanged) {
        m_sitesChanged = sitesChanged;
    }
    void setValuesChanged(bool valuesChanged) {
        m_valuesChanged = valuesChanged;
    }
    void setColorScaleChanged(bool colorScaleChanged) {
        m_colorScaleChanged = colorScaleChanged;
    }

signals:
    void sitesChanged(const arma::mat &sites) const;
    void valuesChanged(const arma::vec &values) const;
    void colorScaleChanged(const ColorScale &scale) const;
    void alphaChanged(float alpha) const;

public slots:
    // 'points' should be a 2D points matrix (each point in a row)
    void setSites(const arma::mat &points);

    // Set the value to be colorScaleped in each site
    void setValues(const arma::vec &values);

    // Set colorScale data based on the given color scale
    void setColorScale(const ColorScale &scale);

    // Shepard blur radius
    Q_INVOKABLE void setAlpha(float alpha);

    // Maximum blur radius
    Q_INVOKABLE void setBeta(float beta);

private:
    std::vector<float> m_sites, m_values, m_cmap;
    float m_alpha, m_beta;
    bool m_sitesChanged, m_valuesChanged, m_colorScaleChanged;
};

#endif // VORONOISPLAT_H
