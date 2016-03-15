#ifndef LINEPLOT_H
#define LINEPLOT_H

#include <memory>
#include <vector>

#include <QQuickFramebufferObject>

#include <armadillo>

// From CUBu
#include "gdrawing.h"

#include "colorscale.h"
#include "scale.h"

class LinePlot
    : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    static const int PADDING = 20;

    LinePlot(QQuickItem *parent = 0);

    void setColorScale(const ColorScale *colorScale);
    void setAutoScale(bool autoScale);

    const GraphDrawing *graphDrawing() const { return m_gdPtr.get(); }
    const std::vector<float> &values() const { return m_values; }
    LinearScale<float> scaleX() const { return m_sx; }
    LinearScale<float> scaleY() const { return m_sy; }

    Renderer *createRenderer() const;

    bool xyChanged() const         { return m_xyChanged; }
    bool valuesChanged() const     { return m_valuesChanged; }
    bool colorScaleChanged() const { return m_colorScaleChanged; }

    void setXYChanged(bool xyChanged) {
        m_xyChanged = xyChanged;
    }
    void setValuesChanged(bool valuesChanged) {
        m_valuesChanged = valuesChanged;
    }
    void setColorScaleChanged(bool colorScaleChanged) {
        m_colorScaleChanged = colorScaleChanged;
    }

signals:
    void xyChanged(const arma::mat &xy);
    void valuesChanged(const arma::vec &values) const;
    void scaleChanged(const LinearScale<float> &sx,
                      const LinearScale<float> &sy) const;

public slots:
    void setXY(const arma::mat &xy);
    void setValues(const arma::vec &values);
    void setScale(const LinearScale<float> &sx,
                  const LinearScale<float> &sy);

private:
    void bundle();

    // Data
    arma::mat m_xy;
    std::vector<float> m_values;

    // Visuals
    const ColorScale *m_colorScale;

    void autoScale();
    bool m_autoScale;
    LinearScale<float> m_sx, m_sy;

    std::unique_ptr<GraphDrawing> m_gdPtr;

    // Internal state
    bool m_xyChanged, m_valuesChanged, m_colorScaleChanged;
    bool m_bundleGPU;
};

#endif // LINEPLOT_H
