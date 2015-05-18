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
        arma::mat projection = getProjection(X);
        Scatterplot *plot = view.rootObject()->findChild<Scatterplot *>("plot", Qt::FindDirectChildrenOnly);
        plot->setData(projection);
    }

    view.show();

    return app.exec();
}
