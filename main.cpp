#include <cmath>
#include <memory>
#include <iostream>
#include <QSurfaceFormat>
#include <QApplication>
#include <QQmlApplicationEngine>

#include "mp.h"
#include "continuouscolorscale.h"
#include "scatterplot.h"
#include "interactionhandler.h"
#include "selectionhandler.h"
#include "effectivenessobserver.h"
#include "distortionobserver.h"
#include "npdistortion.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");

    // Set up multisampling
    QSurfaceFormat fmt;
    fmt.setSamples(16);
    QSurfaceFormat::setDefaultFormat(fmt);
    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

    arma::mat dataset;
    if (argc > 1) {
        dataset.load(argv[1], arma::raw_ascii);
    } else {
        dataset.load(std::cin, arma::raw_ascii);
    }

    arma::mat X = dataset.cols(0, dataset.n_cols - 2);
    arma::vec labels = dataset.col(dataset.n_cols - 1);

    arma::uword n = dataset.n_rows;
    arma::uword subsampleSize = (arma::uword) sqrt(n) * 3;
    arma::uvec sampleIndices = arma::randi<arma::uvec>(subsampleSize, arma::distr_param(0, n-1));
    arma::mat Ys(subsampleSize, 2, arma::fill::randn);
    Ys = mp::forceScheme(mp::dist(X.rows(sampleIndices)), Ys);

    /*
    ColorScale colorScale{
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
    colorScale.setExtents(labels.min(), labels.max());
    */

    ContinuousColorScale colorScale = ContinuousColorScale::builtin(ContinuousColorScale::RED_GRAY_BLUE);
    colorScale.setExtents(-1, 1);
    Scatterplot *subsamplePlot = engine.rootObjects()[0]->findChild<Scatterplot *>("subsamplePlot");
    subsamplePlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    subsamplePlot->setXY(Ys);
    subsamplePlot->setColorData(arma::zeros<arma::vec>(subsampleSize));
    subsamplePlot->setColorScale(&colorScale);
    Scatterplot *plot = engine.rootObjects()[0]->findChild<Scatterplot *>("plot");

    // connect both plots through interaction handler
    InteractionHandler interactionHandler(X, sampleIndices);
    QObject::connect(subsamplePlot, SIGNAL(xyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setSubsample(const arma::mat &)));
    QObject::connect(&interactionHandler, SIGNAL(subsampleChanged(const arma::mat &)),
            plot, SLOT(setXY(const arma::mat &)));

    SelectionHandler selectionHandler(sampleIndices);
    QObject::connect(subsamplePlot, SIGNAL(selectionChanged(const arma::uvec &)),
            &selectionHandler, SLOT(setSelection(const arma::uvec &)));
    QObject::connect(&selectionHandler, SIGNAL(selectionChanged(const arma::uvec &)),
            plot, SLOT(setSelection(const arma::uvec &)));

    DistortionObserver distortionObs(X, sampleIndices);
    std::unique_ptr<DistortionMeasure> distortionMeasure(new NPDistortion());
    distortionObs.setMeasure(distortionMeasure.get());
    QObject::connect(&interactionHandler, SIGNAL(subsampleChanged(const arma::mat &)),
            &distortionObs, SLOT(setMap(const arma::mat &)));
    QObject::connect(&distortionObs, SIGNAL(mapChanged(const arma::vec &)),
            plot, SLOT(setColorData(const arma::vec &)));

    EffectiveInteractionEnforcer enforcer(sampleIndices);
    QObject::connect(subsamplePlot, SIGNAL(selectionChanged(const arma::uvec &)),
            &enforcer, SLOT(setSelection(const arma::uvec &)));
    QObject::connect(plot, SIGNAL(colorDataChanged(const arma::vec &)),
            &enforcer, SLOT(setMeasureDifference(const arma::vec &)));
    QObject::connect(&enforcer, SIGNAL(effectivenessChanged(const arma::vec &)),
            subsamplePlot, SLOT(setColorData(const arma::vec &)));

    ContinuousColorScale ccolorScale = ContinuousColorScale::builtin(ContinuousColorScale::RED_GRAY_BLUE);
    ccolorScale.setExtents(-1, 1);
    plot->setColorScale(&ccolorScale);
    interactionHandler.setSubsample(Ys);

    return app.exec();
}
