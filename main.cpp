#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
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
#include "barchart.h"
#include "colormap.h"
#include "transitioncontrol.h"
#include "manipulationhandler.h"
#include "mapscalehandler.h"
#include "selectionhandler.h"
#include "brushinghandler.h"
#include "projectionobserver.h"

static const int RNG_SEED = 1;

static QObject *mainProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return Main::instance();
}

arma::uvec extractCPs(const arma::mat &X)
{
    int numCPs = (int) (3 * sqrt(X.n_rows));
    arma::uvec cpIndices(numCPs);
    std::iota(cpIndices.begin(), cpIndices.end(), 0);
    std::shuffle(cpIndices.begin(),
                 cpIndices.end(),
                 std::default_random_engine(RNG_SEED));
    return cpIndices;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("pm");
    app.setApplicationVersion("1.0");
    // app.setAttribute(Qt::AA_ShareOpenGLContexts);

    // Command line parser
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

    // Load dataset
    Main *m = Main::instance();
    if (!m->loadDataset(args[0].toStdString())) {
        std::cerr << "Could not load dataset.\n";
        return 1;
    }

    arma::mat X = m->X();
    arma::vec labels = m->labels();

    arma::uvec cpIndices;
    arma::mat Ys;

    // Load/generate indices
    QString indicesFilename;
    if (parser.isSet(indicesFileOutputOption)) {
        indicesFilename = parser.value(indicesFileOutputOption);
    }
    QFile indicesFile(indicesFilename);
    if (indicesFile.exists()) {
        m->setIndicesSavePath(indicesFilename);
        cpIndices.load(indicesFilename.toStdString(), arma::raw_ascii);
    } else {
        std::cerr << "No indices file, generating indices...\n";
        cpIndices = extractCPs(X);
    }

    // Load/generate CPs
    QString cpFilename;
    if (parser.isSet(cpFileOutputOption)) {
        cpFilename = parser.value(cpFileOutputOption);
    }
    QFile cpFile(cpFilename);
    if (cpFile.exists()) {
        m->setCPSavePath(cpFilename);
        Ys.load(cpFilename.toStdString(), arma::raw_ascii);
    } else {
        std::cerr << "No CP file, generating initial projection.\n";
        Ys.set_size(cpIndices.n_elem, 2);
        mp::forceScheme(mp::dist(X.rows(cpIndices)), Ys);
    }

    if (cpIndices.n_elem != Ys.n_rows) {
        std::cerr << "The number of CP indices and the CP map do not match." << std::endl;
        return 1;
    }

    // Sort indices (some operations become easier later)
    arma::uvec cpSortedIndices = arma::sort_index(cpIndices);
    cpIndices = cpIndices(cpSortedIndices);
    Ys = Ys.rows(cpSortedIndices);

    m->setCPIndices(cpIndices);
    m->setCP(Ys);

    // Set up multisampling
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    //fmt.setVersion(4, 5);
    fmt.setRedBufferSize(8);
    fmt.setGreenBufferSize(8);
    fmt.setBlueBufferSize(8);
    fmt.setAlphaBufferSize(8);
    fmt.setSamples(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    // Register our custom QML types & init QML engine
    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");
    qmlRegisterType<BarChart>("PM", 1, 0, "BarChart");
    qmlRegisterType<VoronoiSplat>("PM", 1, 0, "VoronoiSplat");
    qmlRegisterType<Colormap>("PM", 1, 0, "Colormap");
    qmlRegisterType<TransitionControl>("PM", 1, 0, "TransitionControl");
    qmlRegisterSingletonType<Main>("PM", 1, 0, "Main", mainProvider);
    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

    // Initialize pointers to visual components
    m->cpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("cpPlot");
    m->rpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("rpPlot");
    m->colormap = engine.rootObjects()[0]->findChild<Colormap *>("colormap");
    m->splat = engine.rootObjects()[0]->findChild<VoronoiSplat *>("splat");
    m->cpBarChart = engine.rootObjects()[0]->findChild<BarChart *>("cpBarChart");
    m->rpBarChart = engine.rootObjects()[0]->findChild<BarChart *>("rpBarChart");
    TransitionControl *plotTC = engine.rootObjects()[0]->findChild<TransitionControl *>("plotTC");

    // Shared object which stores modifications to projections
    ProjectionHistory history;
    m->projectionHistory = &history;

    // Keep track of the current cp (in order to save them later, if requested)
    QObject::connect(m->cpPlot, &Scatterplot::xyChanged,
            m, &Main::setCP);
    QObject::connect(m->cpPlot, &Scatterplot::xyInteractivelyChanged,
            m, &Main::setCP);

    // Update projection as the cp are modified (either directly in the
    // manipulationHandler object or interactively in cpPlot
    ManipulationHandler manipulationHandler(X, cpIndices, m->projectionHistory);
    QObject::connect(m->cpPlot, &Scatterplot::xyInteractivelyChanged,
            &manipulationHandler, &ManipulationHandler::setCP);

    // Update history whenever a new projection is computed...
    QObject::connect(&manipulationHandler, &ManipulationHandler::mapChanged,
            m->projectionHistory, &ProjectionHistory::addMap);

    // ... and update visual components whenever the history changes
    QObject::connect(m->projectionHistory, &ProjectionHistory::mapAdded,
            m, &Main::updateMap);
    QObject::connect(m->projectionHistory, &ProjectionHistory::undoPerformed,
            m, &Main::updateMap);
    QObject::connect(m->projectionHistory, &ProjectionHistory::resetPerformed,
            m, &Main::updateMap);

    // Keep both scatterplots and the splat scaled equally and relative to the
    // full plot
    MapScaleHandler mapScaleHandler;
    QObject::connect(m->projectionHistory, &ProjectionHistory::mapAdded,
            &mapScaleHandler, &MapScaleHandler::scaleToMap);
    QObject::connect(m->projectionHistory, &ProjectionHistory::undoPerformed,
            &mapScaleHandler, &MapScaleHandler::scaleToMap);
    QObject::connect(m->projectionHistory, &ProjectionHistory::resetPerformed,
            &mapScaleHandler, &MapScaleHandler::scaleToMap);
    QObject::connect(&mapScaleHandler, &MapScaleHandler::scaleChanged,
            m->cpPlot, &Scatterplot::setScale);
    QObject::connect(&mapScaleHandler, &MapScaleHandler::scaleChanged,
            m->rpPlot, &Scatterplot::setScale);
    QObject::connect(&mapScaleHandler, &MapScaleHandler::scaleChanged,
            m->splat, &VoronoiSplat::setScale);

    QObject::connect(m->splat, &VoronoiSplat::colorScaleChanged,
            m->colormap, &Colormap::setColorScale);

    // Linking between selections
    SelectionHandler cpSelectionHandler(cpIndices.n_elem);
    QObject::connect(m->cpPlot, &Scatterplot::selectionInteractivelyChanged,
            &cpSelectionHandler, &SelectionHandler::setSelection);
    QObject::connect(m->cpBarChart, &BarChart::selectionInteractivelyChanged,
            &cpSelectionHandler, &SelectionHandler::setSelection);
    QObject::connect(&cpSelectionHandler, &SelectionHandler::selectionChanged,
            m->cpPlot, &Scatterplot::setSelection);

    SelectionHandler rpSelectionHandler(X.n_rows - cpIndices.n_elem);
    QObject::connect(m->rpPlot, &Scatterplot::selectionInteractivelyChanged,
            &rpSelectionHandler, &SelectionHandler::setSelection);
    QObject::connect(m->rpBarChart, &BarChart::selectionInteractivelyChanged,
            &rpSelectionHandler, &SelectionHandler::setSelection);
    QObject::connect(&rpSelectionHandler, &SelectionHandler::selectionChanged,
            m->rpPlot, &Scatterplot::setSelection);

    // Brushing between bar chart and respective scatterplot
    BrushingHandler cpBrushHandler;
    QObject::connect(m->cpPlot, &Scatterplot::itemInteractivelyBrushed,
            &cpBrushHandler, &BrushingHandler::brushItem);
    QObject::connect(m->cpBarChart, &BarChart::itemInteractivelyBrushed,
            &cpBrushHandler, &BrushingHandler::brushItem);
    QObject::connect(&cpBrushHandler, &BrushingHandler::itemBrushed,
            m->cpPlot, &Scatterplot::brushItem);
    QObject::connect(&cpBrushHandler, &BrushingHandler::itemBrushed,
            m->cpBarChart, &BarChart::brushItem);

    BrushingHandler rpBrushHandler;
    QObject::connect(m->rpPlot, &Scatterplot::itemInteractivelyBrushed,
            &rpBrushHandler, &BrushingHandler::brushItem);
    QObject::connect(m->rpBarChart, &BarChart::itemInteractivelyBrushed,
            &rpBrushHandler, &BrushingHandler::brushItem);
    QObject::connect(&rpBrushHandler, &BrushingHandler::itemBrushed,
            m->rpPlot, &Scatterplot::brushItem);
    QObject::connect(&rpBrushHandler, &BrushingHandler::itemBrushed,
            m->rpBarChart, &BarChart::brushItem);

    // Recompute values whenever projection changes
    ProjectionObserver projectionObserver(X, cpIndices, m->projectionHistory);
    m->projectionObserver = &projectionObserver;
    QObject::connect(m->projectionHistory, &ProjectionHistory::mapAdded,
            m->projectionObserver, &ProjectionObserver::setMap);
    QObject::connect(m->projectionHistory, &ProjectionHistory::undoPerformed,
            m->projectionObserver, &ProjectionObserver::setMap);
    QObject::connect(m->projectionHistory, &ProjectionHistory::resetPerformed,
            m->projectionObserver, &ProjectionObserver::setMap);
    QObject::connect(m->projectionObserver, &ProjectionObserver::cpValuesChanged,
            m->cpPlot, &Scatterplot::setColorData);
    QObject::connect(m->projectionObserver, &ProjectionObserver::rpValuesChanged,
            m->splat, &VoronoiSplat::setValues);
    QObject::connect(m->projectionObserver, &ProjectionObserver::cpValuesChanged,
            m->cpBarChart, &BarChart::setValues);
    QObject::connect(m->projectionObserver, &ProjectionObserver::rpValuesChanged,
            m->rpBarChart, &BarChart::setValues);

    // Recompute values whenever selection changes
    QObject::connect(&cpSelectionHandler, &SelectionHandler::selectionChanged,
            &projectionObserver, &ProjectionObserver::setCPSelection);
    QObject::connect(&rpSelectionHandler, &SelectionHandler::selectionChanged,
            &projectionObserver, &ProjectionObserver::setRPSelection);

    // Connect projection components to rewinding mechanism
    QObject::connect(plotTC, &TransitionControl::tChanged,
            &manipulationHandler, &ManipulationHandler::setRewind);
    QObject::connect(plotTC, &TransitionControl::tChanged,
            m->projectionObserver, &ProjectionObserver::setRewind);

    QObject::connect(&manipulationHandler, &ManipulationHandler::mapRewound,
            m, &Main::updateMap);
    QObject::connect(m->projectionObserver, &ProjectionObserver::cpValuesRewound,
            m->cpPlot, &Scatterplot::setColorData);
    QObject::connect(m->projectionObserver, &ProjectionObserver::rpValuesRewound,
            m->splat, &VoronoiSplat::setValues);

    // General component set up
    plotTC->setAcceptedMouseButtons(Qt::MiddleButton);
    m->cpPlot->setDragEnabled(true);
    m->cpPlot->setAutoScale(false);
    m->rpPlot->setAutoScale(false);
    m->cpBarChart->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    m->setSelectCPs();

    m->cpBarChart->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    m->rpBarChart->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);

    m->setColormapColorScale(Main::ColorScaleRainbow);
    m->setCPPlotColorScale(Main::ColorScaleRainbow);
    m->setRPPlotColorScale(Main::ColorScaleRainbow);
    m->setSplatColorScale(Main::ColorScaleRainbow);
    m->setCPBarChartColorScale(Main::ColorScaleRainbow);
    m->setRPBarChartColorScale(Main::ColorScaleRainbow);


    // This sets the initial CP configuration, triggering all the necessary
    // signals to set up the helper objects and visual components
    manipulationHandler.setCP(Ys);

    return app.exec();
}
