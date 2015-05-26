#include <cmath>
#include <memory>
#include <iostream>
#include <QSurfaceFormat>
#include <QApplication>
#include <QQmlApplicationEngine>

#include "mp.h"
#include "scatterplot.h"
#include "interactionhandler.h"

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
    arma::mat Ys = arma::randn(subsampleSize, 2);
    Ys = mp::forceScheme(mp::dist(X.rows(sampleIndices)), Ys);
    arma::mat subsampleData(subsampleSize, 3);
    subsampleData.cols(0, 1) = Ys;
    subsampleData.col(2) = labels(sampleIndices);

    Scatterplot *subsamplePlot = engine.rootObjects()[0]->findChild<Scatterplot *>("subsamplePlot");
    subsamplePlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    subsamplePlot->setData(subsampleData);
    Scatterplot *plot = engine.rootObjects()[0]->findChild<Scatterplot *>("plot");

    // connect both plots through interaction handler
    std::unique_ptr<InteractionHandler> interactionHandler(new InteractionHandler(X, labels, sampleIndices));
    QObject::connect(subsamplePlot, SIGNAL(dataChanged(const arma::mat &)),
                        interactionHandler.get(), SLOT(setSubsample(const arma::mat &)));
    QObject::connect(interactionHandler.get(), SIGNAL(subsampleChanged(const arma::mat &)),
                        plot, SLOT(setData(const arma::mat &)));
    interactionHandler.get()->setSubsample(Ys);
    return app.exec();
}
