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

    const std::vector<float> &sites() const    { return m_sites; }
    const std::vector<float> &values() const   { return m_values; }
    const std::vector<float> &colormap() const { return m_cmap; }

    bool sitesChanged() const    { return m_sitesChanged; }
    bool valuesChanged() const   { return m_valuesChanged; }
    bool colormapChanged() const { return m_colormapChanged; }

    void setSitesChanged(bool sitesChanged)       { m_sitesChanged = sitesChanged; }
    void setValuesChanged(bool valuesChanged)     { m_valuesChanged = valuesChanged; }
    void setColormapChanged(bool colormapChanged) { m_colormapChanged = colormapChanged; }

signals:
    void sitesChanged(const arma::mat &sites);
    void valuesChanged(const arma::vec &values);
    void colormapChanged(const ColorScale *scale);

public slots:
    // 'points' should be a 2D points matrix (each point in a row)
    void setSites(const arma::mat &points);

    // Set the value to be colormapped in each site
    void setValues(const arma::vec &values);

    // Set colormap data based on the given color scale
    void setColormap(const ColorScale *scale);

private:
    std::vector<float> m_sites, m_values, m_cmap;
    bool m_sitesChanged, m_valuesChanged, m_colormapChanged;
};

#endif // VORONOISPLAT_H
