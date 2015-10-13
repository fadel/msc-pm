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
#include "historygraph.h"
#include "interactionhandler.h"
#include "selectionhandler.h"
#include "effectivenessobserver.h"
#include "distortionobserver.h"
#include "npdistortion.h"

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

    qmlRegisterType<Scatterplot>("PM", 1, 0, "Scatterplot");
    qmlRegisterType<HistoryGraph>("PM", 1, 0, "HistoryGraph");
    qmlRegisterSingletonType<Main>("PM", 1, 0, "Main", mainProvider);

    QCommandLineParser parser;
    parser.setApplicationDescription("Interactive multidimensional projections.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("dataset", "Dataset filename (.tbl)");

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

    // Set up multisampling
    QSurfaceFormat fmt;
    fmt.setSamples(16);
    QSurfaceFormat::setDefaultFormat(fmt);
    QQmlApplicationEngine engine(QUrl("qrc:///main_view.qml"));

    Main *m = Main::instance();
    if (parser.isSet(indicesFileOutputOption)) {
        m->setIndicesSavePath(parser.value(indicesFileOutputOption));
    }
    if (parser.isSet(subsampleFileOutputOption)) {
        m->setSubsampleSavePath(parser.value(subsampleFileOutputOption));
    }
    m->loadDataset(args[0].toStdString());

    arma::mat X = m->X();
    arma::vec labels = m->labels();

    arma::uword n = X.n_rows;
    arma::uword subsampleSize = (arma::uword) n / 10.f;
    arma::uvec sampleIndices = arma::randi<arma::uvec>(subsampleSize, arma::distr_param(0, n-1));
    m->setSubsampleIndices(sampleIndices);

    arma::mat Ys(subsampleSize, 2, arma::fill::randn);
    mp::forceScheme(mp::dist(X.rows(sampleIndices)), Ys);

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
    subsamplePlot->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    // subsamplePlot->setColorData(arma::zeros<arma::vec>(subsampleSize));
    subsamplePlot->setColorScale(&colorScale);
    Scatterplot *plot = engine.rootObjects()[0]->findChild<Scatterplot *>("plot");

    // Keep track of the current subsample (in order to save them later, if requested)
    QObject::connect(subsamplePlot, SIGNAL(xyChanged(const arma::mat &)),
            m, SLOT(setSubsample(const arma::mat &)));
    QObject::connect(subsamplePlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            m, SLOT(setSubsample(const arma::mat &)));

    // Update LAMP projection as the subsample is modified
    InteractionHandler interactionHandler(X, sampleIndices);
    QObject::connect(subsamplePlot, SIGNAL(xyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setSubsample(const arma::mat &)));
    QObject::connect(subsamplePlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            &interactionHandler, SLOT(setSubsample(const arma::mat &)));
    QObject::connect(&interactionHandler, SIGNAL(subsampleChanged(const arma::mat &)),
            plot, SLOT(setXY(const arma::mat &)));

    // Linking between selections in subsample plot and full dataset plot
    SelectionHandler selectionHandler(sampleIndices);
    QObject::connect(subsamplePlot, SIGNAL(selectionChanged(const QSet<int> &)),
            &selectionHandler, SLOT(setSelection(const QSet<int> &)));
    QObject::connect(&selectionHandler, SIGNAL(selectionChanged(const QSet<int> &)),
            plot, SLOT(setSelection(const QSet<int> &)));

    // Connections between history graph and subsample plot
    HistoryGraph *history = engine.rootObjects()[0]->findChild<HistoryGraph *>("history");
    QObject::connect(subsamplePlot, SIGNAL(xyInteractivelyChanged(const arma::mat &)),
            history, SLOT(addHistoryItem(const arma::mat &)));
    QObject::connect(history, SIGNAL(currentItemChanged(const arma::mat &)),
            subsamplePlot, SLOT(setXY(const arma::mat &)));

    // Map distortion as the glyph color
    //DistortionObserver distortionObs(X, sampleIndices);
    //std::unique_ptr<DistortionMeasure> distortionMeasure(new NPDistortion());
    //distortionObs.setMeasure(distortionMeasure.get());
    //QObject::connect(&interactionHandler, SIGNAL(subsampleChanged(const arma::mat &)),
    //        &distortionObs, SLOT(setMap(const arma::mat &)));
    //QObject::connect(&distortionObs, SIGNAL(mapChanged(const arma::vec &)),
    //        plot, SLOT(setColorData(const arma::vec &)));

    //EffectiveInteractionEnforcer enforcer(sampleIndices);
    //QObject::connect(subsamplePlot, SIGNAL(selectionChanged(const QSet<int> &)),
    //        &enforcer, SLOT(setSelection(const QSet<int> &)));
    //QObject::connect(plot, SIGNAL(colorDataChanged(const arma::vec &)),
    //        &enforcer, SLOT(setMeasureDifference(const arma::vec &)));
    //QObject::connect(&enforcer, SIGNAL(effectivenessChanged(const arma::vec &)),
    //        subsamplePlot, SLOT(setColorData(const arma::vec &)));


    history->addHistoryItem(Ys);
    subsamplePlot->setXY(Ys);
    subsamplePlot->setColorData(labels(sampleIndices));
    plot->setColorScale(&colorScale);
    plot->setColorData(labels);

    return app.exec();
}
