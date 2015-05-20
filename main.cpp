#include <cmath>
#include <memory>
#include <QSurfaceFormat>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "mp.h"
#include "scatterplot.h"
#include "interactionhandler.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");

    QSurfaceFormat fmt;
    fmt.setSamples(16);
    QSurfaceFormat::setDefaultFormat(fmt);
    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

    std::unique_ptr<InteractionHandler> interactionHandler((InteractionHandler *) 0);
    if (argc > 1) {
        arma::mat dataset;
        dataset.load(argv[1], arma::raw_ascii);
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
        interactionHandler = std::unique_ptr<InteractionHandler>(new InteractionHandler(X, labels, sampleIndices));
        QObject::connect(subsamplePlot, SIGNAL(dataChanged(const arma::mat &)),
                         interactionHandler.get(), SLOT(setSubsample(const arma::mat &)));
        QObject::connect(interactionHandler.get(), SIGNAL(subsampleChanged(const arma::mat &)),
                         plot, SLOT(setData(const arma::mat &)));

    }

    return app.exec();
}
