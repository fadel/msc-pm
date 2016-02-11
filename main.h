#ifndef MAIN_H
#define MAIN_H

#include <QObject>
#include <armadillo>

#include "barchart.h"
#include "colormap.h"
#include "colorscale.h"
#include "continuouscolorscale.h"
#include "projectionobserver.h"
#include "projectionhistory.h"
#include "scatterplot.h"
#include "voronoisplat.h"

class Main : public QObject
{
    Q_OBJECT
    Q_ENUMS(ObserverType)
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

    Q_INVOKABLE void setSelectRPs() {
        cpPlot->setAcceptedMouseButtons(Qt::NoButton);
        cpPlot->setAcceptHoverEvents(false);

        rpPlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
        rpPlot->setAcceptHoverEvents(true);
    }

    Q_INVOKABLE void setSelectCPs() {
        rpPlot->setAcceptedMouseButtons(Qt::NoButton);
        rpPlot->setAcceptHoverEvents(false);

        cpPlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
        cpPlot->setAcceptHoverEvents(true);
    }

    enum ObserverType {
        ObserverCurrent      = ProjectionObserver::OBSERVER_CURRENT,
        ObserverDiffPrevious = ProjectionObserver::OBSERVER_DIFF_PREVIOUS,
        ObserverDiffOriginal = ProjectionObserver::OBSERVER_DIFF_ORIGINAL
    };

    Q_INVOKABLE bool setObserverType(ObserverType observerType) {
        return projectionObserver->setType((int) observerType);
    }

    enum ColorScaleType {
        ColorScaleCategorical,
        ColorScaleContinuous,
        ColorScaleDivergent,
        ColorScaleRainbow
    };

    // No need to be static: this class is a singleton
    ColorScale COLOR_SCALE_CATEGORICAL;
    ColorScale COLOR_SCALE_CONTINUOUS;
    ColorScale COLOR_SCALE_DIVERGENT;
    ColorScale COLOR_SCALE_RAINBOW;

    Q_INVOKABLE void setCPPlotColorScale(ColorScaleType colorScaleType) {
        cpPlot->setColorScale(getColorScale(colorScaleType));
    }

    Q_INVOKABLE void setRPPlotColorScale(ColorScaleType colorScaleType) {
        rpPlot->setColorScale(getColorScale(colorScaleType));
    }

    Q_INVOKABLE void setColormapColorScale(ColorScaleType colorScaleType) {
        colormap->setColorScale(getColorScale(colorScaleType));
    }

    Q_INVOKABLE void setCPBarChartColorScale(ColorScaleType colorScaleType) {
        cpBarChart->setColorScale(getColorScale(colorScaleType));
    }

    Q_INVOKABLE void setRPBarChartColorScale(ColorScaleType colorScaleType) {
        rpBarChart->setColorScale(getColorScale(colorScaleType));
    }

    Q_INVOKABLE void setSplatColorScale(ColorScaleType colorScaleType) {
        splat->setColorScale(getColorScale(colorScaleType));
    }

    // Pointers to visual components whose values are set in the main() function
    // after components are instantiated by the QtQuick engine
    BarChart *cpBarChart, *rpBarChart;
    Colormap *colormap;
    Scatterplot *cpPlot, *rpPlot;
    VoronoiSplat *splat;

    ProjectionObserver *projectionObserver;

    // Shared object that controls manipulation history
    ProjectionHistory *projectionHistory;
    Q_INVOKABLE void undoManipulation()  { projectionHistory->undo(); }
    Q_INVOKABLE void resetManipulation() { projectionHistory->undoAll(); }

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
        , COLOR_SCALE_CATEGORICAL{{
            QColor("#1f77b4"), QColor("#ff7f0e"), QColor("#2ca02c"),
            QColor("#d62728"), QColor("#9467bd"), QColor("#8c564b"),
            QColor("#e377c2"), QColor("#17becf"), QColor("#7f7f7f"),
          }}
        , COLOR_SCALE_CONTINUOUS{ContinuousColorScale::builtin(ContinuousColorScale::HEATED_OBJECTS)}
        , COLOR_SCALE_DIVERGENT{ContinuousColorScale::builtin(ContinuousColorScale::RED_GRAY_BLUE)}
        , COLOR_SCALE_RAINBOW{ContinuousColorScale::builtin(ContinuousColorScale::RAINBOW)}
        , cpBarChart(0)
        , rpBarChart(0)
        , cpPlot(0)
        , rpPlot(0)
        , splat(0)
    {
    }
    ~Main() {}

    ColorScale &getColorScale(ColorScaleType colorScaleType) {
        switch (colorScaleType) {
        case ColorScaleCategorical:
            return COLOR_SCALE_CATEGORICAL;
        case ColorScaleContinuous:
            return COLOR_SCALE_CONTINUOUS;
        case ColorScaleDivergent:
            return COLOR_SCALE_DIVERGENT;
        case ColorScaleRainbow:
            // fall-through
        default:
            return COLOR_SCALE_RAINBOW;
        }
    }

    arma::mat m_dataset, m_cp;
    arma::uvec m_cpIndices;
    std::string m_indicesSavePath, m_cpSavePath;
};

#endif // MAIN_H
