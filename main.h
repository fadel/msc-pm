#ifndef MAIN_H
#define MAIN_H

#include <QObject>
#include <armadillo>
#include <memory>

#include "colorscale.h"
#include "continuouscolorscale.h"
#include "divergentcolorscale.h"
#include "projectionhistory.h"
#include "numericrange.h"
#include "barchart.h"
#include "colormap.h"
#include "lineplot.h"
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

    Q_INVOKABLE bool loadDataset(const std::string &dataPath) {
        return m_X.load(dataPath, arma::raw_ascii);
    }

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

    arma::mat X() const { return m_X; }

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

    enum ColorScaleType {
        ColorScaleCategorical,
        ColorScaleContinuous,
        ColorScaleDivergent,
        ColorScaleRainbow
    };

    Q_INVOKABLE void setCPColorScale(ColorScaleType colorScaleType) {
        ColorScale *ptr = colorScaleCPs.get();
        float min = ptr != nullptr ? ptr->min() : 0.0f;
        float max = ptr != nullptr ? ptr->max() : 1.0f;

        ptr = getColorScale(colorScaleType);
        ptr->setExtents(min, max);
        colorScaleCPs.reset(ptr);

        cpPlot->setColorScale(colorScaleCPs.get());
        cpBarChart->setColorScale(colorScaleCPs.get());
        cpColormap->setColorScale(colorScaleCPs.get());
        bundlePlot->setColorScale(colorScaleCPs.get());
    }

    Q_INVOKABLE void setRPColorScale(ColorScaleType colorScaleType) {
        ColorScale *ptr = colorScaleCPs.get();
        float min = 0.0f;
        float max = 1.0f;
        if (ptr) {
            min = ptr->min();
            max = ptr->max();
        }
        ptr = getColorScale(colorScaleType);
        ptr->setExtents(min, max);
        colorScaleRPs.reset(ptr);

        rpPlot->setColorScale(colorScaleRPs.get());
        splat->setColorScale(colorScaleRPs.get());
        rpBarChart->setColorScale(colorScaleRPs.get());
        rpColormap->setColorScale(colorScaleRPs.get());
    }

    // Pointers to visual components whose values are set in the main() function
    // after components are instantiated by the QtQuick engine
    BarChart *cpBarChart, *rpBarChart;
    Colormap *cpColormap, *rpColormap;
    Scatterplot *cpPlot, *rpPlot;
    VoronoiSplat *splat;
    LinePlot *bundlePlot;

    // Color scales in use
    std::unique_ptr<ColorScale> colorScaleCPs, colorScaleRPs;

    // Object that controls manipulation history
    ProjectionHistory *projectionHistory;

    Q_INVOKABLE void undoManipulation()  { projectionHistory->undo(); }
    Q_INVOKABLE void resetManipulation() { projectionHistory->reset(); }

    enum ObserverType {
        ObserverCurrent      = ProjectionHistory::ObserverCurrent,
        ObserverDiffPrevious = ProjectionHistory::ObserverDiffPrevious,
        ObserverDiffFirst    = ProjectionHistory::ObserverDiffFirst
    };

    Q_INVOKABLE bool setObserverType(ObserverType observerType) {
        switch (observerType) {
        case ObserverCurrent:
            return projectionHistory->setType(ProjectionHistory::ObserverCurrent);
        case ObserverDiffPrevious:
            return projectionHistory->setType(ProjectionHistory::ObserverDiffPrevious);
        case ObserverDiffFirst:
            return projectionHistory->setType(ProjectionHistory::ObserverDiffFirst);
        }

        return false;
    }

public slots:
    void setCPIndices(const arma::uvec &indices) {
        m_cpIndices = indices;

        m_rpIndices.set_size(m_X.n_rows - m_cpIndices.n_elem);
        NumericRange<arma::uword> allIndices(0, m_X.n_rows);
        std::set_symmetric_difference(allIndices.cbegin(), allIndices.cend(),
                m_cpIndices.cbegin(), m_cpIndices.cend(), m_rpIndices.begin());
    }

    void setCP(const arma::mat &cp) {
        if (cp.n_cols != 2
            || cp.n_rows != m_cpIndices.n_elem) {
            return;
        }

        m_cp = cp;
    }

    void updateMap(const arma::mat &Y) {
        cpPlot->setXY(Y.rows(m_cpIndices));

        const arma::mat &regularPoints = Y.rows(m_rpIndices);
        rpPlot->setXY(regularPoints);
        splat->setSites(regularPoints);
    }

private:
    Main(QObject *parent = 0)
        : QObject(parent)
        , cpBarChart(0)
        , rpBarChart(0)
        , cpColormap(0)
        , rpColormap(0)
        , cpPlot(0)
        , rpPlot(0)
        , splat(0)
        , bundlePlot(0)
        , projectionHistory(0)
    {
    }

    ~Main() {}

    ColorScale *getColorScale(ColorScaleType colorScaleType) {
        switch (colorScaleType) {
        case ColorScaleCategorical:
            return new ColorScale{
                    QColor("#1f77b4"), QColor("#ff7f0e"), QColor("#2ca02c"),
                    QColor("#d62728"), QColor("#9467bd"), QColor("#8c564b"),
                    QColor("#e377c2"), QColor("#17becf"), QColor("#7f7f7f"),
                  };
        case ColorScaleContinuous:
            return ContinuousColorScale::builtin(
                        ContinuousColorScale::HeatedObjects, nullptr);
        case ColorScaleDivergent:
            return DivergentColorScale::builtin(
                        DivergentColorScale::RedGrayBlue, nullptr);
        case ColorScaleRainbow:
            // fall-through
        default:
            return ContinuousColorScale::builtin(
                        ContinuousColorScale::Rainbow, nullptr);
        }
    }

    arma::mat m_X, m_cp;
    arma::uvec m_cpIndices, m_rpIndices;
    std::string m_indicesSavePath, m_cpSavePath;
};

#endif // MAIN_H
