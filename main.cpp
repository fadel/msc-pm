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
#include "interactionhandler.h"
#include "selectionhandler.h"
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
        "Filename to store the subsample indices. Omitting this option disables saving indices.",
        "filename");
    parser.addOption(indicesFileOutputOption);
    QCommandLineOption subsampleFileOutputOption(QStringList() << "s" << "subsample",
        "Filename to store subsample mapping. Omitting this option disables saving subsamples.",
        "filename");
    parser.addOption(subsampleFileOutputOption);

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
    int subsampleSize = (int) (3 * sqrt(n));
    arma::uvec sampleIndices;
    arma::mat Ys;

    if (parser.isSet(indicesFileOutputOption)) {
        const QString &indicesFilename = parser.value(indicesFileOutputOption);
        m->setIndicesSavePath(indicesFilename);
        QFile indicesFile(indicesFilename);
        if (indicesFile.exists()) {
            sampleIndices.load(indicesFilename.toStdString(), arma::raw_ascii);
            subsampleSize = sampleIndices.n_elem;
        } else {
            // sampleIndices = relevanceSampling(X, subsampleSize);
            sampleIndices = arma::randi<arma::uvec>(subsampleSize, arma::distr_param(0, n-1));
        }
    }
    if (parser.isSet(subsampleFileOutputOption)) {
        const QString &subsampleFilename = parser.value(subsampleFileOutputOption);
        m->setSubsampleSavePath(subsampleFilename);
        QFile subsampleFile(subsampleFilename);
        if (subsampleFile.exists()) {
            Ys.load(subsampleFilename.toStdString(), arma::raw_ascii);
        } else {
            Ys.set_size(subsampleSize, 2);
            mp::forceScheme(mp::dist(X.rows(sampleIndices)), Ys);
        }
    }

    m->setSubsampleIndices(sampleIndices);
    m->setSubsample(Ys);

    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");
    qmlRegisterType<HistoryGraph>("PM", 1, 0, "HistoryGraph");
    qmlRegisterType<BarChart>("PM", 1, 0, "BarChart");
    qmlRegisterType<InteractionHandler>("PM", 1, 0, "InteractionHandler");
    qmlRegisterSingletonType<Main>("PM", 1, 0, "Main", mainProvider);

    // Set up multisampling
    QSurfaceFormat fmt;
    fmt.setSamples(16);
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(4, 5);
    QSurfaceFormat::setDefaultFormat(fmt);

    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

    ColorScale colorScale{
        QColor("#1f77b4"),
        QColor("#ff7f0e"),
        QColor("#2ca02c"),
        QColor("#d62728"),
        QColor("#9467bd"),
        QColor("#8c564b"),
        QColor("#e377c2"),
        QColor("#17becf"),
        QColor("#7f7f7f"),
    };
    colorScale.setExtents(labels.min(), labels.max());

    //ContinuousColorScale colorScale = ContinuousColorScale::builtin(ContinuousColorScale::RED_GRAY_BLUE);
    //colorScale.setExtents(-1, 1);
    Scatterplot *subsamplePlot = engine.rootObjects()[0]->findChild<Scatterplot *>("subsamplePlot");
    subsamplePlot->setDisplaySplat(false);
    subsamplePlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    // subsamplePlot->setColorData(arma::zeros<arma::vec>(subsampleSize));
    subsamplePlot->setColorScale(&colorScale);
    Scatterplot *plot = engine.rootObjects()[0]->findChild<Scatterplot *>("plot");
    skelft2DInitialization(plot->width());

    // Keep track of the current subsample (in order to save them later, if requested)
    QObject::connect(subsamplePlot, SIGNAL(xyChanged(const arma::mat &)),
            m, SLOT(setSubsample(const arma::mat &)));
    QObject::connect(subsamplePlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            m, SLOT(setSubsample(const arma::mat &)));

    // Update projection as the subsample is modified
    InteractionHandler interactionHandler(X, sampleIndices);
    m->setInteractionHandler(&interactionHandler);
    QObject::connect(subsamplePlot, SIGNAL(xyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setSubsample(const arma::mat &)));
    QObject::connect(subsamplePlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setSubsample(const arma::mat &)));
    QObject::connect(&interactionHandler, SIGNAL(subsampleChanged(const arma::mat &)),
            plot, SLOT(setXY(const arma::mat &)));
    m->setTechnique(InteractionHandler::TECHNIQUE_LAMP);

    // Linking between selections in subsample plot and full dataset plot
    SelectionHandler selectionHandler(sampleIndices);
    QObject::connect(subsamplePlot, SIGNAL(selectionChanged(const QSet<int> &)),
            &selectionHandler, SLOT(setSelection(const QSet<int> &)));
    QObject::connect(&selectionHandler, SIGNAL(selectionChanged(const QSet<int> &)),
            plot, SLOT(setSelection(const QSet<int> &)));

    // Connections between history graph and subsample plot
    //HistoryGraph *history = engine.rootObjects()[0]->findChild<HistoryGraph *>("history");
    //QObject::connect(subsamplePlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
    //        history, SLOT(addHistoryItem(const arma::mat &)));
    //QObject::connect(history, SIGNAL(currentItemChanged(const arma::mat &)),
    //        subsamplePlot, SLOT(setXY(const arma::mat &)));

    BarChart *barChart = engine.rootObjects()[0]->findChild<BarChart *>("barChart");
    barChart->setValues(arma::randn<arma::vec>(100));

    //history->addHistoryItem(Ys);
    plot->setColorScale(&colorScale);
    plot->setColorData(labels, false);
    subsamplePlot->setColorData(labels(sampleIndices), false);
    subsamplePlot->setXY(Ys, false);
    subsamplePlot->update();

    auto ret = app.exec();

    skelft2DDeinitialization();
    return ret;
}
