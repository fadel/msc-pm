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
    m->loadDataset(args[0].toStdString());
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

    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");
    qmlRegisterType<BarChart>("PM", 1, 0, "BarChart");
    qmlRegisterType<VoronoiSplat>("PM", 1, 0, "VoronoiSplat");
    qmlRegisterType<Colormap>("PM", 1, 0, "Colormap");
    qmlRegisterSingletonType<Main>("PM", 1, 0, "Main", mainProvider);

    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

    // Initialize pointers to visual components
    m->cpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("cpPlot");
    m->rpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("rpPlot");
    m->colormap = engine.rootObjects()[0]->findChild<Colormap *>("colormap");
    m->splat = engine.rootObjects()[0]->findChild<VoronoiSplat *>("splat");
    m->cpBarChart = engine.rootObjects()[0]->findChild<BarChart *>("cpBarChart");
    m->rpBarChart = engine.rootObjects()[0]->findChild<BarChart *>("rpBarChart");

    // Keep track of the current cp (in order to save them later, if requested)
    QObject::connect(m->cpPlot, SIGNAL(xyChanged(const arma::mat &)),
            m, SLOT(setCP(const arma::mat &)));
    QObject::connect(m->cpPlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            m, SLOT(setCP(const arma::mat &)));

    // Update projection as the cp are modified (either directly in the
    // manipulationHandler object or interactively in cpPlot
    ManipulationHandler manipulationHandler(X, cpIndices);
    QObject::connect(m->cpPlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            &manipulationHandler, SLOT(setCP(const arma::mat &)));
    QObject::connect(&manipulationHandler, SIGNAL(cpChanged(const arma::mat &)),
            m->cpPlot, SLOT(setXY(const arma::mat &)));
    QObject::connect(&manipulationHandler, SIGNAL(rpChanged(const arma::mat &)),
            m->rpPlot, SLOT(setXY(const arma::mat &)));
    QObject::connect(&manipulationHandler, SIGNAL(rpChanged(const arma::mat &)),
            m->splat, SLOT(setSites(const arma::mat &)));

    // Keep both scatterplots and the splat scaled equally and relative to the
    // full plot
    MapScaleHandler mapScaleHandler;
    QObject::connect(&mapScaleHandler, SIGNAL(scaleChanged(const LinearScale<float> &, const LinearScale<float> &)),
            m->cpPlot, SLOT(setScale(const LinearScale<float> &, const LinearScale<float> &)));
    QObject::connect(&mapScaleHandler, SIGNAL(scaleChanged(const LinearScale<float> &, const LinearScale<float> &)),
            m->rpPlot, SLOT(setScale(const LinearScale<float> &, const LinearScale<float> &)));
    QObject::connect(&mapScaleHandler, SIGNAL(scaleChanged(const LinearScale<float> &, const LinearScale<float> &)),
            m->splat, SLOT(setScale(const LinearScale<float> &, const LinearScale<float> &)));
    QObject::connect(&manipulationHandler, SIGNAL(mapChanged(const arma::mat &)),
            &mapScaleHandler, SLOT(scaleToMap(const arma::mat &)));

    QObject::connect(m->splat, SIGNAL(colorScaleChanged(const ColorScale &)),
            m->colormap, SLOT(setColorScale(const ColorScale &)));

    // Linking between selections
    SelectionHandler cpSelectionHandler(cpIndices.n_elem);
    QObject::connect(m->cpPlot, SIGNAL(selectionInteractivelyChanged(const std::vector<bool> &)),
            &cpSelectionHandler, SLOT(setSelection(const std::vector<bool> &)));
    QObject::connect(m->cpBarChart, SIGNAL(selectionInteractivelyChanged(const std::vector<bool> &)),
            &cpSelectionHandler, SLOT(setSelection(const std::vector<bool> &)));
    QObject::connect(&cpSelectionHandler, SIGNAL(selectionChanged(const std::vector<bool> &)),
            m->cpPlot, SLOT(setSelection(const std::vector<bool> &)));

    SelectionHandler rpSelectionHandler(X.n_rows - cpIndices.n_elem);
    QObject::connect(m->rpPlot, SIGNAL(selectionInteractivelyChanged(const std::vector<bool> &)),
            &rpSelectionHandler, SLOT(setSelection(const std::vector<bool> &)));
    QObject::connect(m->rpBarChart, SIGNAL(selectionInteractivelyChanged(const std::vector<bool> &)),
            &rpSelectionHandler, SLOT(setSelection(const std::vector<bool> &)));
    QObject::connect(&rpSelectionHandler, SIGNAL(selectionChanged(const std::vector<bool> &)),
            m->rpPlot, SLOT(setSelection(const std::vector<bool> &)));

    // Brushing between bar chart and respective scatterplot
    BrushingHandler cpBrushHandler;
    QObject::connect(m->cpPlot, SIGNAL(itemInteractivelyBrushed(int)),
            &cpBrushHandler, SLOT(brushItem(int)));
    QObject::connect(m->cpBarChart, SIGNAL(itemInteractivelyBrushed(int)),
            &cpBrushHandler, SLOT(brushItem(int)));
    QObject::connect(&cpBrushHandler, SIGNAL(itemBrushed(int)),
            m->cpPlot, SLOT(brushItem(int)));
    QObject::connect(&cpBrushHandler, SIGNAL(itemBrushed(int)),
            m->cpBarChart, SLOT(brushItem(int)));

    BrushingHandler rpBrushHandler;
    QObject::connect(m->rpPlot, SIGNAL(itemInteractivelyBrushed(int)),
            &rpBrushHandler, SLOT(brushItem(int)));
    QObject::connect(m->rpBarChart, SIGNAL(itemInteractivelyBrushed(int)),
            &rpBrushHandler, SLOT(brushItem(int)));
    QObject::connect(&rpBrushHandler, SIGNAL(itemBrushed(int)),
            m->rpPlot, SLOT(brushItem(int)));
    QObject::connect(&rpBrushHandler, SIGNAL(itemBrushed(int)),
            m->rpBarChart, SLOT(brushItem(int)));

    // Recompute values whenever projection changes
    ProjectionObserver projectionObserver(X, cpIndices);
    m->projectionObserver = &projectionObserver;
    QObject::connect(&manipulationHandler, SIGNAL(mapChanged(const arma::mat &)),
            m->projectionObserver, SLOT(setMap(const arma::mat &)));
    QObject::connect(m->projectionObserver, SIGNAL(cpValuesChanged(const arma::vec &)),
            m->cpPlot, SLOT(setColorData(const arma::vec &)));
    QObject::connect(m->projectionObserver, SIGNAL(rpValuesChanged(const arma::vec &)),
            m->splat, SLOT(setValues(const arma::vec &)));
    QObject::connect(m->projectionObserver, SIGNAL(cpValuesChanged(const arma::vec &)),
            m->cpBarChart, SLOT(setValues(const arma::vec &)));
    QObject::connect(m->projectionObserver, SIGNAL(rpValuesChanged(const arma::vec &)),
            m->rpBarChart, SLOT(setValues(const arma::vec &)));

    // Recompute values whenever selection changes
    QObject::connect(&cpSelectionHandler, SIGNAL(selectionChanged(const std::vector<bool> &)),
            &projectionObserver, SLOT(setCPSelection(const std::vector<bool> &)));
    QObject::connect(&rpSelectionHandler, SIGNAL(selectionChanged(const std::vector<bool> &)),
            &projectionObserver, SLOT(setRPSelection(const std::vector<bool> &)));

    // General component set up
    m->cpPlot->setAcceptHoverEvents(true);
    m->cpPlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    m->cpBarChart->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    m->rpBarChart->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);

    m->setColormapColorScale(Main::ColorScaleRainbow);
    m->setCPPlotColorScale(Main::ColorScaleRainbow);
    m->setRPPlotColorScale(Main::ColorScaleRainbow);
    m->setSplatColorScale(Main::ColorScaleRainbow);
    m->setCPBarChartColorScale(Main::ColorScaleRainbow);
    m->setRPBarChartColorScale(Main::ColorScaleRainbow);

    m->cpPlot->setAutoScale(false);
    m->rpPlot->setAutoScale(false);
    m->rpPlot->setGlyphSize(3.0f);

    // This sets the initial CP configuration, triggering all the necessary
    // signals to set up the helper objects and visual components
    manipulationHandler.setCP(Ys);

    return app.exec();
}
