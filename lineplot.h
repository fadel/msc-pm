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

    // Main bundling
    Q_PROPERTY(int   iterations          READ iterations          WRITE setIterations          NOTIFY iterationsChanged)
    Q_PROPERTY(float kernelSize          READ kernelSize          WRITE setKernelSize          NOTIFY kernelSizeChanged)
    Q_PROPERTY(float smoothingFactor     READ smoothingFactor     WRITE setSmoothingFactor     NOTIFY smoothingFactorChanged)
    Q_PROPERTY(int   smoothingIterations READ smoothingIterations WRITE setSmoothingIterations NOTIFY smoothingIterationsChanged)

    // Ends bundling
    Q_PROPERTY(bool  blockEndpoints      READ blockEndpoints      WRITE setBlockEndpoints      NOTIFY blockEndpointsChanged)
    Q_PROPERTY(int   endsIterations      READ endsIterations      WRITE setEndsIterations      NOTIFY endsIterationsChanged)
    Q_PROPERTY(float endsKernelSize      READ endsKernelSize      WRITE setEndsKernelSize      NOTIFY endsKernelSizeChanged)
    Q_PROPERTY(float endsSmoothingFactor READ endsSmoothingFactor WRITE setEndsSmoothingFactor NOTIFY endsSmoothingFactorChanged)

    // General bundling options
    Q_PROPERTY(float edgeSampling   READ edgeSampling   WRITE setEdgeSampling   NOTIFY edgeSamplingChanged)
    Q_PROPERTY(float advectionSpeed READ advectionSpeed WRITE setAdvectionSpeed NOTIFY advectionSpeedChanged)
    Q_PROPERTY(float relaxation     READ relaxation     WRITE setRelaxation     NOTIFY relaxationChanged)
    Q_PROPERTY(bool  bundleGPU      READ bundleGPU      WRITE setBundleGPU      NOTIFY bundleGPUChanged)

public:
    static const int PADDING = 20;

    LinePlot(QQuickItem *parent = 0);

    void setColorScale(const ColorScale *scale);

    const GraphDrawing *graphDrawing()     const { return &m_gdFinal; }
    const std::vector<float> &values()     const { return m_values; }
    const std::vector<float> &colorScale() const { return m_cmap; }
    LinearScale<float> scaleX() const { return m_sx; }
    LinearScale<float> scaleY() const { return m_sy; }

    Renderer *createRenderer() const;

    bool linesChanged()      const { return m_linesChanged; }
    bool valuesChanged()     const { return m_valuesChanged; }
    bool colorScaleChanged() const { return m_colorScaleChanged; }

    void setLinesChanged(bool linesChanged) {
        m_linesChanged = linesChanged;
    }
    void setValuesChanged(bool valuesChanged) {
        m_valuesChanged = valuesChanged;
    }
    void setColorScaleChanged(bool colorScaleChanged) {
        m_colorScaleChanged = colorScaleChanged;
    }

    // Q_PROPERTY's
    int   iterations()          const { return m_iterations; }
    float kernelSize()          const { return m_kernelSize; }
    float smoothingFactor()     const { return m_smoothingFactor; }
    int   smoothingIterations() const { return m_smoothingIterations; }

    void setIterations(int iterations);
    void setKernelSize(float kernelSize);
    void setSmoothingFactor(float smoothingFactor);
    void setSmoothingIterations(int smoothingIterations);

    bool  blockEndpoints()      const { return m_blockEndpoints; }
    int   endsIterations()      const { return m_endsIterations; }
    float endsKernelSize()      const { return m_endsKernelSize; }
    float endsSmoothingFactor() const { return m_endsSmoothingFactor; }

    void setBlockEndpoints(bool blockEndpoints);
    void setEndsIterations(int endsIterations);
    void setEndsKernelSize(float endsKernelSize);
    void setEndsSmoothingFactor(float endsSmoothingFactor);

    float edgeSampling()   const { return m_edgeSampling; }
    float advectionSpeed() const { return m_advectionSpeed; }
    float relaxation()     const { return m_relaxation; }
    bool  bundleGPU()      const { return m_bundleGPU; }

    void setEdgeSampling(float edgeSampling);
    void setAdvectionSpeed(float advectionSpeed);
    void setRelaxation(float relaxation);
    void setBundleGPU(bool bundleGPU);

signals:
    void linesChanged(const arma::mat &xy);
    void valuesChanged(const arma::vec &values) const;
    void scaleChanged(const LinearScale<float> &sx,
                      const LinearScale<float> &sy) const;

    // Q_PROPERTY's
    void iterationsChanged(int iterations) const;
    void kernelSizeChanged(float kernelSize) const;
    void smoothingFactorChanged(float smoothingFactor) const;
    void smoothingIterationsChanged(int smoothingIterations) const;

    void blockEndpointsChanged(bool blockEndpoints) const;
    void endsIterationsChanged(int endsIterations) const;
    void endsKernelSizeChanged(float endsKernelSize) const;
    void endsSmoothingFactorChanged(float endsSmoothingFactor) const;

    void edgeSamplingChanged(float edgeSampling) const;
    void advectionSpeedChanged(float advectionSpeed) const;
    void relaxationChanged(float relaxation) const;
    void bundleGPUChanged(bool bundleGPU) const;

public slots:
    // Lines are two consecutive elements in 'indices' (ref. points in 'Y')
    void setLines(const arma::uvec &indices, const arma::mat &Y);

    // One value for each line
    void setValues(const arma::vec &values);

    void setScale(const LinearScale<float> &sx,
                  const LinearScale<float> &sy);

private:
    void bundle();
    void relax();

    // Data
    arma::mat m_lines;
    std::vector<float> m_values;

    // Visuals
    std::vector<float> m_cmap;
    LinearScale<float> m_sx, m_sy;
    std::unique_ptr<GraphDrawing> m_gdPtr;
    GraphDrawing m_gdBundle, m_gdFinal;

    // Internal state
    bool m_linesChanged, m_valuesChanged, m_colorScaleChanged;

    // Q_PROPERTY's
    int   m_iterations;
    float m_kernelSize;
    float m_smoothingFactor;
    int   m_smoothingIterations;

    bool  m_blockEndpoints;
    int   m_endsIterations;
    float m_endsKernelSize;
    float m_endsSmoothingFactor;

    float m_edgeSampling;
    float m_advectionSpeed;
    float m_relaxation;
    bool  m_bundleGPU;
};

#endif // LINEPLOT_H
