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
#include "lineplot.h"
#include "barchart.h"
#include "colormap.h"
#include "transitioncontrol.h"
#include "projectionhistory.h"
#include "manipulationhandler.h"
#include "mapscalehandler.h"
#include "selectionhandler.h"
#include "brushinghandler.h"

static const int RNG_SEED = 123;

static QObject *mainProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return Main::instance();
}

arma::uvec extractCPs(const arma::mat &X)
{
    int numCPs = (int) (3 * sqrt(X.n_rows));
    arma::uvec indices(X.n_rows);
    std::iota(indices.begin(), indices.end(), 0);
    indices = arma::shuffle(indices);
    return indices.subvec(0, numCPs-1);
}

void overviewBundles(const Main *m)
{
    const arma::mat &unreliability = m->projectionHistory->unreliability();
    arma::uvec indicesLargest = arma::sort_index(unreliability, "descending");
    arma::uword numLargest = m->projectionHistory->Y().n_rows * 0.1f;
    indicesLargest = indicesLargest.subvec(0, numLargest-1);
    m->bundlePlot->setValues(unreliability(indicesLargest));

    const arma::uvec &cpIndices = m->projectionHistory->cpIndices();
    arma::uvec CPs = cpIndices(indicesLargest / unreliability.n_rows);

    const arma::uvec &rpIndices = m->projectionHistory->rpIndices();
    arma::uvec RPs = indicesLargest;
    RPs.transform([&unreliability](arma::uword v) {
        return v % unreliability.n_rows;
    });
    RPs = rpIndices(RPs);

    arma::uvec indices(CPs.n_elem + RPs.n_elem);
    for (arma::uword i = 0; i < CPs.n_elem; i++) {
        indices[2*i + 0] = CPs(i);
        indices[2*i + 1] = RPs(i);
    }
    m->bundlePlot->setLines(indices, m->projectionHistory->Y());
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

    const arma::mat &X = m->X();
    //arma::vec labels = m->labels();

    arma::arma_rng::set_seed(RNG_SEED);
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
        std::cerr << "No CP file, generating initial projection...\n";
        Ys.set_size(cpIndices.n_elem, 2);
        Ys.randu();
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
    qmlRegisterType<LinePlot>("PM", 1, 0, "LinePlot");
    qmlRegisterType<Colormap>("PM", 1, 0, "Colormap");
    qmlRegisterType<TransitionControl>("PM", 1, 0, "TransitionControl");
    qmlRegisterSingletonType<Main>("PM", 1, 0, "Main", mainProvider);
    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

    // Initialize pointers to visual components
    m->cpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("cpPlot");
    m->rpPlot = engine.rootObjects()[0]->findChild<Scatterplot *>("rpPlot");
    m->splat = engine.rootObjects()[0]->findChild<VoronoiSplat *>("splat");
    m->bundlePlot = engine.rootObjects()[0]->findChild<LinePlot *>("bundlePlot");
    m->cpColormap = engine.rootObjects()[0]->findChild<Colormap *>("cpColormap");
    m->rpColormap = engine.rootObjects()[0]->findChild<Colormap *>("rpColormap");
    m->cpBarChart = engine.rootObjects()[0]->findChild<BarChart *>("cpBarChart");
    m->rpBarChart = engine.rootObjects()[0]->findChild<BarChart *>("rpBarChart");
    TransitionControl *plotTC = engine.rootObjects()[0]->findChild<TransitionControl *>("plotTC");

    // Shared object which stores modifications to projections
    ProjectionHistory history(X, cpIndices);
    m->projectionHistory = &history;

    // Keep track of the current cp (in order to save them later, if requested)
    QObject::connect(m->cpPlot, &Scatterplot::xyChanged,
            m, &Main::setCP);
    QObject::connect(m->cpPlot, &Scatterplot::xyInteractivelyChanged,
            m, &Main::setCP);

    // Keep both scatterplots, the splat and line plot scaled equally and
    // relative to the full plot
    MapScaleHandler mapScaleHandler;
    QObject::connect(&mapScaleHandler, &MapScaleHandler::scaleChanged,
            m->cpPlot, &Scatterplot::setScale);
    QObject::connect(&mapScaleHandler, &MapScaleHandler::scaleChanged,
            m->rpPlot, &Scatterplot::setScale);
    QObject::connect(&mapScaleHandler, &MapScaleHandler::scaleChanged,
            m->splat, &VoronoiSplat::setScale);
    QObject::connect(&mapScaleHandler, &MapScaleHandler::scaleChanged,
            m->bundlePlot, &LinePlot::setScale);
    QObject::connect(m->projectionHistory, &ProjectionHistory::currentMapChanged,
            &mapScaleHandler, &MapScaleHandler::scaleToMap);

    // Update projection as the cp are modified (either directly in the
    // manipulationHandler object or interactively in cpPlot
    ManipulationHandler manipulationHandler(X, cpIndices);
    QObject::connect(m->cpPlot, &Scatterplot::xyInteractivelyChanged,
            &manipulationHandler, &ManipulationHandler::setCP);

    // Update history whenever a new projection is computed...
    QObject::connect(&manipulationHandler, &ManipulationHandler::mapChanged,
            m->projectionHistory, &ProjectionHistory::addMap);

    // ... and update visual components whenever the history changes
    QObject::connect(m->projectionHistory, &ProjectionHistory::currentMapChanged,
            m, &Main::updateMap);
    QObject::connect(m->projectionHistory, &ProjectionHistory::currentMapChanged,
            [m](const arma::mat &Y) {
                // ... and bundling
                overviewBundles(m);
            });

    // Linking between selections
    SelectionHandler cpSelectionHandler(cpIndices.n_elem);
    QObject::connect(m->cpPlot, &Scatterplot::selectionInteractivelyChanged,
            &cpSelectionHandler, &SelectionHandler::setSelection);
    QObject::connect(m->cpBarChart, &BarChart::selectionInteractivelyChanged,
            &cpSelectionHandler, &SelectionHandler::setSelection);
    QObject::connect(&cpSelectionHandler, &SelectionHandler::selectionChanged,
            m->cpPlot, &Scatterplot::setSelection);
    QObject::connect(&cpSelectionHandler, &SelectionHandler::selectionChanged,
            m->cpBarChart, &BarChart::setSelection);
    QObject::connect(&cpSelectionHandler, &SelectionHandler::selectionChanged,
            [m](const std::vector<bool> &cpSelection) {
                // given some CPs, see unexpected RPs *influenced by* them
                std::vector<arma::uword> selectedCPIndices;
                for (int i = 0; i < cpSelection.size(); i++) {
                    if (cpSelection[i]) {
                        selectedCPIndices.push_back(i);
                    }
                }

                if (selectedCPIndices.empty()) {
                    overviewBundles(m);
                    return;
                }

                arma::uvec selectedCPs(selectedCPIndices.size());
                std::copy(selectedCPIndices.begin(), selectedCPIndices.end(),
                        selectedCPs.begin());

                // Only the 10% largest values, filtered by the selected RPs
                const arma::mat &unreliability = m->projectionHistory->unreliability();
                arma::uvec indicesLargest = arma::sort_index(unreliability.cols(selectedCPs), "descending");
                arma::uword numLargest = indicesLargest.n_elem * 0.1f;
                indicesLargest = indicesLargest.subvec(0, numLargest-1);
                m->bundlePlot->setValues(unreliability(indicesLargest));

                const arma::uvec &cpIndices = m->projectionHistory->cpIndices();
                arma::uvec CPs = cpIndices(selectedCPs(indicesLargest / unreliability.n_rows));

                const arma::uvec &rpIndices = m->projectionHistory->rpIndices();
                arma::uvec RPs = indicesLargest;
                RPs.transform([&unreliability](arma::uword v) {
                    return v % unreliability.n_rows;
                });
                RPs = rpIndices(RPs);

                arma::uvec indices(CPs.n_elem + RPs.n_elem);
                for (arma::uword i = 0; i < CPs.n_elem; i++) {
                    indices[2*i + 0] = CPs(i);
                    indices[2*i + 1] = RPs(i);
                }
                m->bundlePlot->setLines(indices, m->projectionHistory->Y());
            });

    SelectionHandler rpSelectionHandler(X.n_rows - cpIndices.n_elem);
    QObject::connect(m->rpPlot, &Scatterplot::selectionInteractivelyChanged,
            &rpSelectionHandler, &SelectionHandler::setSelection);
    QObject::connect(m->rpBarChart, &BarChart::selectionInteractivelyChanged,
            &rpSelectionHandler, &SelectionHandler::setSelection);
    QObject::connect(&rpSelectionHandler, &SelectionHandler::selectionChanged,
            m->rpPlot, &Scatterplot::setSelection);
    QObject::connect(&rpSelectionHandler, &SelectionHandler::selectionChanged,
            m->rpBarChart, &BarChart::setSelection);
    QObject::connect(&rpSelectionHandler, &SelectionHandler::selectionChanged,
            [m](const std::vector<bool> &rpSelection) {
                // given some RPs, see unexpected CPs *influencing* them
                std::vector<arma::uword> selectedRPIndices;
                for (int i = 0; i < rpSelection.size(); i++) {
                    if (rpSelection[i]) {
                        selectedRPIndices.push_back(i);
                    }
                }

                if (selectedRPIndices.empty()) {
                    overviewBundles(m);
                    return;
                }

                arma::uvec selectedRPs(selectedRPIndices.size());
                std::copy(selectedRPIndices.begin(), selectedRPIndices.end(),
                        selectedRPs.begin());

                // Only the 10% largest values, filtered by the selected RPs
                const arma::mat &unreliability = m->projectionHistory->unreliability();
                arma::uvec indicesLargest = arma::sort_index(unreliability.rows(selectedRPs), "descending");
                arma::uword numLargest = indicesLargest.n_elem * 0.1f;
                indicesLargest = indicesLargest.subvec(0, numLargest-1);
                m->bundlePlot->setValues(unreliability(indicesLargest));

                const arma::uvec &cpIndices = m->projectionHistory->cpIndices();
                arma::uvec CPs = cpIndices(indicesLargest / selectedRPs.n_elem);

                const arma::uvec &rpIndices = m->projectionHistory->rpIndices();
                arma::uvec RPs = indicesLargest;
                RPs.transform([&selectedRPs](arma::uword v) {
                    return v % selectedRPs.n_elem;
                });
                RPs = rpIndices(selectedRPs(RPs));

                arma::uvec indices(CPs.n_elem + RPs.n_elem);
                for (arma::uword i = 0; i < CPs.n_elem; i++) {
                    indices[2*i + 0] = CPs(i);
                    indices[2*i + 1] = RPs(i);
                }
                m->bundlePlot->setLines(indices, m->projectionHistory->Y());
            });

    // Brushing between each bar chart and respective scatterplot
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

    // Update visual components whenever values change
    QObject::connect(m->projectionHistory, &ProjectionHistory::rpValuesChanged,
            [m](const arma::vec &v, bool rescale) {
                ColorScale *ptr = m->colorScaleRPs.get();
                if (!ptr || v.n_elem == 0) {
                    return;
                }

                if (rescale) {
                    ptr->setExtents(v.min(), v.max());
                } else {
                    ptr->setExtents(std::min(ptr->min(), (float) v.min()),
                                    std::max(ptr->max(), (float) v.max()));
                }

                m->splat->setColorScale(ptr);
                m->rpColormap->setColorScale(ptr);
            });
    QObject::connect(m->projectionHistory, &ProjectionHistory::cpValuesChanged,
             [m](const arma::vec &v) {
                 ColorScale *ptr = m->colorScaleCPs.get();
                 if (!ptr || v.n_elem == 0) {
                     return;
                 }

                 ptr->setExtents(v.min(), v.max());
                 m->cpColormap->setColorScale(ptr);
             });
    QObject::connect(m->projectionHistory, &ProjectionHistory::cpValuesChanged,
            m->cpPlot, &Scatterplot::setColorData);
    QObject::connect(m->projectionHistory, &ProjectionHistory::rpValuesChanged,
            m->splat, &VoronoiSplat::setValues);
    QObject::connect(m->projectionHistory, &ProjectionHistory::cpValuesChanged,
            m->cpBarChart, &BarChart::setValues);
    QObject::connect(m->projectionHistory, &ProjectionHistory::rpValuesChanged,
            m->rpBarChart, &BarChart::setValues);

    // ProjectionHistory takes special care of separate CP/RP selections
    QObject::connect(&cpSelectionHandler, &SelectionHandler::selectionChanged,
            m->projectionHistory, &ProjectionHistory::setCPSelection);
    QObject::connect(&rpSelectionHandler, &SelectionHandler::selectionChanged,
            m->projectionHistory, &ProjectionHistory::setRPSelection);
    //QObject::connect(m->projectionHistory, &ProjectionHistory::selectionChanged,
    //        m->bundlePlot, LinePlot::selectionChanged);

    // Connect projection components to rewinding mechanism
    QObject::connect(plotTC, &TransitionControl::tChanged,
            m->projectionHistory, &ProjectionHistory::setRewind);
    QObject::connect(m->projectionHistory, &ProjectionHistory::mapRewound,
            m, &Main::updateMap);
    QObject::connect(m->projectionHistory, &ProjectionHistory::cpValuesRewound,
            m->cpPlot, &Scatterplot::setColorData);
    QObject::connect(m->projectionHistory, &ProjectionHistory::rpValuesRewound,
            m->splat, &VoronoiSplat::setValues);
    QObject::connect(m->projectionHistory, &ProjectionHistory::rpValuesRewound,
            m->rpBarChart, &BarChart::updateValues);

    // General component set up
    plotTC->setAcceptedMouseButtons(Qt::RightButton);
    m->cpPlot->setDragEnabled(true);
    m->cpPlot->setAutoScale(false);
    m->rpPlot->setAutoScale(false);
    m->cpBarChart->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    m->setSelectCPs();

    m->cpBarChart->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    m->rpBarChart->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);

    m->setCPColorScale(Main::ColorScaleRainbow);
    m->setRPColorScale(Main::ColorScaleRainbow);

    // This sets the initial CP configuration, triggering all the necessary
    // signals to set up the helper objects and visual components
    manipulationHandler.setCP(Ys);

    return app.exec();
}
