#include <cmath>
#include <QSurfaceFormat>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "mp.h"
#include "scatterplot.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");

    /*QQuickView view;
    QSurfaceFormat format = view.format();
    format.setSamples(16);
    view.setFormat(format);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///main_view.qml"));*/
    QSurfaceFormat fmt;
    fmt.setSamples(16);
    QSurfaceFormat::setDefaultFormat(fmt);
    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

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

        // Plot the subsample
        Scatterplot *plot = engine.rootObjects()[0]->findChild<Scatterplot *>("subsamplePlot");
        arma::mat subsampleData(subsampleSize, 3);
        subsampleData.cols(0, 1) = Ys;
        subsampleData.col(2) = labels(sampleIndices);
        plot->setData(subsampleData);

        // Plot entire dataset
        plot = engine.rootObjects()[0]->findChild<Scatterplot *>("plot");
        arma::mat reducedData(n, 3);
        reducedData.cols(0, 1) = mp::lamp(X, sampleIndices, Ys);
        reducedData.col(2) = labels;
        plot->setData(reducedData);
    }

    return app.exec();
}
