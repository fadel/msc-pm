#ifndef MAIN_H
#define MAIN_H

#include <QObject>
#include <armadillo>

#include "colorscale.h"
#include "continuouscolorscale.h"

#include "barchart.h"
#include "colormap.h"
#include "scatterplot.h"
#include "voronoisplat.h"

class Main : public QObject
{
    Q_OBJECT
    Q_ENUMS(ColorScaleType)
public:
    static Main *instance() {
        // FIXME: Possibly dangerous
        static Main *m = 0;
        if (m == 0) {
            m = new Main();
        }

        return m;
    }

    Q_INVOKABLE bool saveData() const {
        bool ret = true;
        if (m_cp.n_elem > 0 && m_indicesSavePath.size() > 0) {
            ret = ret && m_cp.save(m_cpSavePath, arma::raw_ascii);
        }
        if (m_cpIndices.n_elem > 0 && m_cpSavePath.size() > 0) {
            ret = ret && m_cpIndices.save(m_indicesSavePath, arma::raw_ascii);
        }

        return ret;
    }

    Q_INVOKABLE bool loadDataset(const std::string &path) { return m_dataset.load(path, arma::raw_ascii); }

    Q_INVOKABLE void setIndicesSavePath(const std::string &path) {
        m_indicesSavePath = path;
    }
    Q_INVOKABLE void setIndicesSavePath(const QString &path) {
        setIndicesSavePath(path.toStdString());
    }
    Q_INVOKABLE void setCPSavePath(const std::string &path) {
        m_cpSavePath = path;
    }
    Q_INVOKABLE void setCPSavePath(const QString &path) {
        setCPSavePath(path.toStdString());
    }

    arma::mat X() const { return m_dataset.cols(0, m_dataset.n_cols - 2); }
    arma::vec labels() const { return m_dataset.col(m_dataset.n_cols - 1); }

    enum ColorScaleType {
        ColorScaleCategorical,
        ColorScaleContinuous,
        ColorScaleDivergent,
        ColorScaleRainbow
    };

    ColorScale *COLOR_SCALE_CATEGORICAL;
    ColorScale *COLOR_SCALE_CONTINUOUS;
    ColorScale *COLOR_SCALE_DIVERGENT;
    ColorScale *COLOR_SCALE_RAINBOW;

    Q_INVOKABLE void setCPPlotColorScale(ColorScaleType colorScaleType) {
        switch (colorScaleType) {
        case ColorScaleCategorical:
            cpPlot->setColorScale(*COLOR_SCALE_CATEGORICAL);
            break;
        case ColorScaleContinuous:
            cpPlot->setColorScale(*COLOR_SCALE_CONTINUOUS);
            break;
        case ColorScaleDivergent:
            cpPlot->setColorScale(*COLOR_SCALE_DIVERGENT);
            break;
        case ColorScaleRainbow:
            cpPlot->setColorScale(*COLOR_SCALE_RAINBOW);
            break;
        }
    }

    Q_INVOKABLE void setRPPlotColorScale(ColorScaleType colorScaleType) {
        switch (colorScaleType) {
        case ColorScaleCategorical:
            rpPlot->setColorScale(*COLOR_SCALE_CATEGORICAL);
            break;
        case ColorScaleContinuous:
            rpPlot->setColorScale(*COLOR_SCALE_CONTINUOUS);
            break;
        case ColorScaleDivergent:
            rpPlot->setColorScale(*COLOR_SCALE_DIVERGENT);
            break;
        case ColorScaleRainbow:
            rpPlot->setColorScale(*COLOR_SCALE_RAINBOW);
            break;
        }
    }

    Q_INVOKABLE void setColormapColorScale(ColorScaleType colorScaleType) {
        switch (colorScaleType) {
        case ColorScaleCategorical:
            colormap->setColorScale(*COLOR_SCALE_CATEGORICAL);
            break;
        case ColorScaleContinuous:
            colormap->setColorScale(*COLOR_SCALE_CONTINUOUS);
            break;
        case ColorScaleDivergent:
            colormap->setColorScale(*COLOR_SCALE_DIVERGENT);
            break;
        case ColorScaleRainbow:
            colormap->setColorScale(*COLOR_SCALE_RAINBOW);
            break;
        }
    }

    Q_INVOKABLE void setBarChartColorScale(ColorScaleType colorScaleType) {
        switch (colorScaleType) {
        case ColorScaleCategorical:
            barChart->setColorScale(*COLOR_SCALE_CATEGORICAL);
            break;
        case ColorScaleContinuous:
            barChart->setColorScale(*COLOR_SCALE_CONTINUOUS);
            break;
        case ColorScaleDivergent:
            barChart->setColorScale(*COLOR_SCALE_DIVERGENT);
            break;
        case ColorScaleRainbow:
            barChart->setColorScale(*COLOR_SCALE_RAINBOW);
            break;
        }
    }

    Q_INVOKABLE void setSplatColorScale(ColorScaleType colorScaleType) {
        switch (colorScaleType) {
        case ColorScaleCategorical:
            splat->setColorScale(*COLOR_SCALE_CATEGORICAL);
            break;
        case ColorScaleContinuous:
            splat->setColorScale(*COLOR_SCALE_CONTINUOUS);
            break;
        case ColorScaleDivergent:
            splat->setColorScale(*COLOR_SCALE_DIVERGENT);
            break;
        case ColorScaleRainbow:
            splat->setColorScale(*COLOR_SCALE_RAINBOW);
            break;
        }
    }

    BarChart *barChart;
    Colormap *colormap;
    Scatterplot *cpPlot, *rpPlot;
    VoronoiSplat *splat;

public slots:
    void setCPIndices(const arma::uvec &indices) { m_cpIndices = indices; }
    void setCP(const arma::mat &cp) {
        if (cp.n_cols != 2
            || cp.n_rows != m_cpIndices.n_elem) {
            return;
        }

        m_cp = cp;
    }

private:
    Main(QObject *parent = 0)
        : QObject(parent)
        , barChart(0)
        , cpPlot(0)
        , rpPlot(0)
        , splat(0)
    {
        COLOR_SCALE_CATEGORICAL = new ColorScale{
            QColor("#1f77b4"),
            QColor("#ff7f0e"),
            QColor("#2ca02c"),
            QColor("#d62728"),
            QColor("#9467bd"),
            QColor("#8c564b"),
            QColor("#e377c2"),
            QColor("#17becf"),
            QColor("#7f7f7f"),
        };
        COLOR_SCALE_CONTINUOUS =
            new ColorScale{ContinuousColorScale::builtin(ContinuousColorScale::HEATED_OBJECTS)};
        COLOR_SCALE_DIVERGENT =
            new ColorScale{ContinuousColorScale::builtin(ContinuousColorScale::RED_GRAY_BLUE)};
        COLOR_SCALE_RAINBOW =
            new ColorScale{ContinuousColorScale::builtin(ContinuousColorScale::RAINBOW)};
    }
    ~Main() {
        delete COLOR_SCALE_CATEGORICAL;
        delete COLOR_SCALE_CONTINUOUS;
        delete COLOR_SCALE_DIVERGENT;
        delete COLOR_SCALE_RAINBOW;
    }

    arma::mat m_dataset, m_cp;
    arma::uvec m_cpIndices;
    std::string m_indicesSavePath, m_cpSavePath;
};

#endif // MAIN_H
