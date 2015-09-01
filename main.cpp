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
    if (argc > 1)
        dataset.load(argv[1], arma::raw_ascii);
    else
        dataset.load(std::cin, arma::raw_ascii);

    arma::mat X = dataset.cols(0, dataset.n_cols - 2);
    arma::vec labels = dataset.col(dataset.n_cols - 1);

    arma::uword n = dataset.n_rows;
    arma::uword subsampleSize = (arma::uword) sqrt(n) * 3;
    arma::uvec sampleIndices = arma::randi<arma::uvec>(subsampleSize, arma::distr_param(0, n-1));
    arma::mat Ys(subsampleSize, 2, arma::fill::randn);
    Ys = mp::forceScheme(mp::dist(X.rows(sampleIndices)), Ys);

    Scatterplot *subsamplePlot = engine.rootObjects()[0]->findChild<Scatterplot *>("subsamplePlot");
    subsamplePlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    subsamplePlot->setXY(Ys);
    subsamplePlot->setColorData(labels(sampleIndices));
    Scatterplot *plot = engine.rootObjects()[0]->findChild<Scatterplot *>("plot");

    // connect both plots through interaction handler
    InteractionHandler interactionHandler(X, sampleIndices);
    QObject::connect(subsamplePlot, SIGNAL(xyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setSubsample(const arma::mat &)));
    QObject::connect(&interactionHandler, SIGNAL(subsampleChanged(const arma::mat &)),
            plot, SLOT(setXY(const arma::mat &)));

    DistortionObserver distortionObs(X, sampleIndices);
    std::unique_ptr<DistortionMeasure> distortionMeasure(new NPDistortion());
    distortionObs.setMeasure(distortionMeasure.get());
    QObject::connect(&interactionHandler, SIGNAL(subsampleChanged(const arma::mat &)),
            &distortionObs, SLOT(setMap(const arma::mat &)));
    QObject::connect(&distortionObs, SIGNAL(mapChanged(const arma::vec &)),
            plot, SLOT(setColorData(const arma::vec &)));

    ContinuousColorScale colorScale;
    colorScale.setExtents(-1, 1);
    plot->setColorScale(&colorScale);
    interactionHandler.setSubsample(Ys);

    // TODO: remove when proper measure coloring is done
    // plot->setColorData(labels);

    return app.exec();
}
