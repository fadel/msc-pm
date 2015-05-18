#include <cmath>
#include <QGuiApplication>
#include <QtQuick/QQuickView>

#include "mp.h"
#include "scatterplot.h"

arma::uvec getSample(arma::uword n)
{
    return arma::randi<arma::uvec>((arma::uword) 3*sqrt(n), arma::distr_param(0, n-1));
}

arma::mat getProjection(const arma::mat &X)
{
    arma::uword n = X.n_rows;
    arma::uvec sampleIndices = getSample(n);
    arma::mat Ys = arma::randn(sampleIndices.n_elem, 2);
    Ys = mp::forceScheme(mp::dist(X.rows(sampleIndices)), Ys);
    return mp::lamp(X, sampleIndices, Ys);
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");

    QQuickView view;
    QSurfaceFormat format = view.format();
    format.setSamples(16);
    view.setFormat(format);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///main_view.qml"));

    if (argc > 1) {
        arma::mat X;
        X.load(argv[1], arma::raw_ascii);

        Scatterplot *plot = view.rootObject()->findChild<Scatterplot *>("plot");
        arma::mat scatterData(X.n_rows, 3);
        scatterData.cols(0, 1) = getProjection(X.cols(0, X.n_cols - 2));
        scatterData.col(2) = X.col(X.n_cols - 1);
        plot->setData(scatterData);
    }

    view.show();

    return app.exec();
}
