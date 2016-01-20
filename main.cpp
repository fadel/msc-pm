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

arma::uvec extractCPs(const arma::mat &X)
{
    int numCPs = (int) (3 * sqrt(X.n_rows));
    return arma::randi<arma::uvec>(numCPs, arma::distr_param(0, X.n_rows - 1));
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

    arma::uvec cpIndices;
    arma::mat Ys;

    QString indicesFilename;
    if (parser.isSet(indicesFileOutputOption)) {
        indicesFilename = parser.value(indicesFileOutputOption);
    }
    QFile indicesFile(indicesFilename);
    if (indicesFile.exists()) {
        m->setIndicesSavePath(indicesFilename);
        cpIndices.load(indicesFilename.toStdString(), arma::raw_ascii);
    } else {
        cpIndices = extractCPs(X);
    }

    arma::sort(cpIndices);

    QString cpFilename;
    if (parser.isSet(cpFileOutputOption)) {
        cpFilename = parser.value(cpFileOutputOption);
    }
    QFile cpFile(cpFilename);
    if (cpFile.exists()) {
        m->setCPSavePath(cpFilename);
        Ys.load(cpFilename.toStdString(), arma::raw_ascii);
    } else {
        Ys.set_size(cpIndices.n_elem, 2);
        mp::forceScheme(mp::dist(X.rows(cpIndices)), Ys);
    }
    if (cpIndices.n_elem != Ys.n_rows) {
        std::cerr << "The number of CP indices and the CP map do not match." << std::endl;
        return 1;
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

    m->cpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("cpPlot");
    m->cpPlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);

    m->rpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("rpPlot");

    m->splat = engine.rootObjects()[0]->findChild<VoronoiSplat *>("splat");
    skelft2DInitialization(m->splat->width());

    m->colormap = engine.rootObjects()[0]->findChild<Colormap *>("colormap");

    // Keep track of the current cp (in order to save them later, if requested)
    QObject::connect(m->cpPlot, SIGNAL(xyChanged(const arma::mat &)),
            m, SLOT(setCP(const arma::mat &)));
    QObject::connect(m->cpPlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            m, SLOT(setCP(const arma::mat &)));

    // Update projection as the cp are modified
    InteractionHandler interactionHandler(X, cpIndices);
    interactionHandler.setTechnique(InteractionHandler::TECHNIQUE_LAMP);
    QObject::connect(m->cpPlot, SIGNAL(xyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setCP(const arma::mat &)));
    QObject::connect(m->cpPlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setCP(const arma::mat &)));
    QObject::connect(&interactionHandler, SIGNAL(cpChanged(const arma::mat &)),
            m->rpPlot, SLOT(setXY(const arma::mat &)));
    QObject::connect(&interactionHandler, SIGNAL(cpChanged(const arma::mat &)),
            m->splat, SLOT(setSites(const arma::mat &)));

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

    QObject::connect(m->rpPlot, SIGNAL(scaleChanged(const LinearScale<float> &, const LinearScale<float> &)),
            m->cpPlot, SLOT(setScale(const LinearScale<float> &, const LinearScale<float> &)));

    QObject::connect(m->splat, SIGNAL(colorScaleChanged(const ColorScale &)),
            m->colormap, SLOT(setColorScale(const ColorScale &)));

    m->barChart = engine.rootObjects()[0]->findChild<BarChart *>("barChart");
    m->barChart->setAcceptedMouseButtons(Qt::LeftButton);
    m->setBarChartColorScale(Main::ColorScaleContinuous);

    ProjectionObserver projectionObserver(X, cpIndices);
    m->projectionObserver = &projectionObserver;
    QObject::connect(&interactionHandler, SIGNAL(cpChanged(const arma::mat &)),
            m->projectionObserver, SLOT(setMap(const arma::mat &)));
    QObject::connect(m->projectionObserver, SIGNAL(valuesChanged(const arma::vec &)),
            m->rpPlot, SLOT(setColorData(const arma::vec &)));
    QObject::connect(m->projectionObserver, SIGNAL(valuesChanged(const arma::vec &)),
            m->splat, SLOT(setValues(const arma::vec &)));
    QObject::connect(m->projectionObserver, SIGNAL(valuesChanged(const arma::vec &)),
            m->barChart, SLOT(setValues(const arma::vec &)));

    //history->addHistoryItem(Ys);
    m->setColormapColorScale(Main::ColorScaleContinuous);
    m->setCPPlotColorScale(Main::ColorScaleContinuous);
    m->setRPPlotColorScale(Main::ColorScaleContinuous);
    m->setSplatColorScale(Main::ColorScaleContinuous);

    m->cpPlot->setAutoScale(false);
    m->cpPlot->setColorData(labels(cpIndices), false);
    m->cpPlot->setXY(Ys, false);
    m->cpPlot->update();

    auto ret = app.exec();

    skelft2DDeinitialization();
    return ret;
}
