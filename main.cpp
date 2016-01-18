#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include <QApplication>
#include <QtQml>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>

#include "main.h"
#include "mp.h"
#include "continuouscolorscale.h"
#include "scatterplot.h"
#include "voronoisplat.h"
#include "historygraph.h"
#include "barchart.h"
#include "colormap.h"
#include "interactionhandler.h"
#include "projectionobserver.h"
#include "skelft.h"

static QObject *mainProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return Main::instance();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("pm");
    app.setApplicationVersion("1.0");
    // app.setAttribute(Qt::AA_ShareOpenGLContexts);

    QCommandLineParser parser;
    parser.setApplicationDescription("Interactive multidimensional projections.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("dataset", "Dataset filename (.tbl file)");

    QCommandLineOption indicesFileOutputOption(QStringList() << "i" << "indices",
        "Filename to store the control points' indices. Omitting this option disables saving indices.",
        "filename");
    parser.addOption(indicesFileOutputOption);
    QCommandLineOption cpFileOutputOption(QStringList() << "c" << "cpoints",
        "Filename to store the control points' map. Omitting this option disables saving this map.",
        "filename");
    parser.addOption(cpFileOutputOption);

    parser.process(app);
    QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        parser.showHelp(1);
    }

    Main *m = Main::instance();
    m->loadDataset(args[0].toStdString());
    arma::mat X = m->X();
    arma::vec labels = m->labels();

    arma::uword n = X.n_rows;
    int cpSize;
    arma::uvec cpIndices;
    arma::mat Ys;

    if (parser.isSet(indicesFileOutputOption)) {
        const QString &indicesFilename = parser.value(indicesFileOutputOption);
        m->setIndicesSavePath(indicesFilename);
        QFile indicesFile(indicesFilename);
        if (indicesFile.exists()) {
            cpIndices.load(indicesFilename.toStdString(), arma::raw_ascii);
            cpSize = cpIndices.n_elem;
        } else {
            cpSize = (int) (3 * sqrt(n));
            // cpIndices = relevanceSampling(X, cpSize);
            cpIndices = arma::randi<arma::uvec>(cpSize, arma::distr_param(0, n-1));
        }

        arma::sort(cpIndices);
    }
    if (parser.isSet(cpFileOutputOption)) {
        const QString &cpFilename = parser.value(cpFileOutputOption);
        m->setCPSavePath(cpFilename);
        QFile cpFile(cpFilename);
        if (cpFile.exists()) {
            Ys.load(cpFilename.toStdString(), arma::raw_ascii);
        } else {
            Ys.set_size(cpSize, 2);
            mp::forceScheme(mp::dist(X.rows(cpIndices)), Ys);
        }
    }

    m->setCPIndices(cpIndices);
    m->setCP(Ys);

    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");
    qmlRegisterType<HistoryGraph>("PM", 1, 0, "HistoryGraph");
    qmlRegisterType<BarChart>("PM", 1, 0, "BarChart");
    qmlRegisterType<VoronoiSplat>("PM", 1, 0, "VoronoiSplat");
    qmlRegisterType<Colormap>("PM", 1, 0, "Colormap");
    qmlRegisterType<InteractionHandler>("PM", 1, 0, "InteractionHandler");
    qmlRegisterSingletonType<Main>("PM", 1, 0, "Main", mainProvider);

    // Set up multisampling
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(4, 5);
    fmt.setRedBufferSize(8);
    fmt.setGreenBufferSize(8);
    fmt.setBlueBufferSize(8);
    fmt.setAlphaBufferSize(8);
    fmt.setSamples(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

    //ColorScale colorScale{
    //    QColor("#1f77b4"),
    //    QColor("#ff7f0e"),
    //    QColor("#2ca02c"),
    //    QColor("#d62728"),
    //    QColor("#9467bd"),
    //    QColor("#8c564b"),
    //    QColor("#e377c2"),
    //    QColor("#17becf"),
    //    QColor("#7f7f7f"),
    //};
    //colorScale.setExtents(labels.min(), labels.max());

    ContinuousColorScale colorScale = ContinuousColorScale::builtin(ContinuousColorScale::HEATED_OBJECTS);

    Scatterplot *cpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("cpPlot");
    Scatterplot *rpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("rpPlot");
    cpPlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    // cpPlot->setColorData(arma::zeros<arma::vec>(cpSize));
    VoronoiSplat *splat = engine.rootObjects()[0]->findChild<VoronoiSplat *>("splat");
    skelft2DInitialization(splat->width());
    Colormap *colormap = engine.rootObjects()[0]->findChild<Colormap *>("colormap");

    // Keep track of the current cp (in order to save them later, if requested)
    QObject::connect(cpPlot, SIGNAL(xyChanged(const arma::mat &)),
            m, SLOT(setCP(const arma::mat &)));
    QObject::connect(cpPlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            m, SLOT(setCP(const arma::mat &)));

    // Update projection as the cp are modified
    InteractionHandler interactionHandler(X, cpIndices);
    m->setInteractionHandler(&interactionHandler);
    m->setTechnique(InteractionHandler::TECHNIQUE_LAMP);
    QObject::connect(cpPlot, SIGNAL(xyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setCP(const arma::mat &)));
    QObject::connect(cpPlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setCP(const arma::mat &)));
    QObject::connect(&interactionHandler, SIGNAL(cpChanged(const arma::mat &)),
            rpPlot, SLOT(setXY(const arma::mat &)));
    QObject::connect(&interactionHandler, SIGNAL(cpChanged(const arma::mat &)),
            splat, SLOT(setSites(const arma::mat &)));

    // Linking between selections in cp plot and rp plot
    //SelectionHandler selectionHandler(cpIndices);
    //QObject::connect(cpPlot, SIGNAL(selectionChanged(const QSet<int> &)),
    //        &selectionHandler, SLOT(setSelection(const QSet<int> &)));
    //QObject::connect(&selectionHandler, SIGNAL(selectionChanged(const QSet<int> &)),
    //        plot, SLOT(setSelection(const QSet<int> &)));

    // Connections between history graph and cp plot
    //HistoryGraph *history = engine.rootObjects()[0]->findChild<HistoryGraph *>("history");
    //QObject::connect(cpPlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
    //        history, SLOT(addHistoryItem(const arma::mat &)));
    //QObject::connect(history, SIGNAL(currentItemChanged(const arma::mat &)),
    //        cpPlot, SLOT(setXY(const arma::mat &)));

    QObject::connect(rpPlot, SIGNAL(scaleChanged(const LinearScale<float> &, const LinearScale<float> &)),
            cpPlot, SLOT(setScale(const LinearScale<float> &, const LinearScale<float> &)));

    BarChart *barChart = engine.rootObjects()[0]->findChild<BarChart *>("barChart");
    barChart->setAcceptedMouseButtons(Qt::LeftButton);
    barChart->setColorScale(colorScale);

    ProjectionObserver projectionObserver(X, cpIndices);
    QObject::connect(&interactionHandler, SIGNAL(cpChanged(const arma::mat &)),
            &projectionObserver, SLOT(setMap(const arma::mat &)));
    QObject::connect(&projectionObserver, SIGNAL(mapChanged(const arma::vec &)),
            rpPlot, SLOT(setColorData(const arma::vec &)));
    QObject::connect(&projectionObserver, SIGNAL(mapChanged(const arma::vec &)),
            splat, SLOT(setValues(const arma::vec &)));
    QObject::connect(&projectionObserver, SIGNAL(mapChanged(const arma::vec &)),
            barChart, SLOT(setValues(const arma::vec &)));

    //history->addHistoryItem(Ys);
    colormap->setColorScale(colorScale);
    rpPlot->setColorScale(colorScale);
    splat->setColorScale(colorScale);

    cpPlot->setColorScale(colorScale);
    cpPlot->setAutoScale(false);
    cpPlot->setColorData(labels(cpIndices), false);
    cpPlot->setXY(Ys, false);
    cpPlot->update();

    auto ret = app.exec();

    skelft2DDeinitialization();
    return ret;
}
