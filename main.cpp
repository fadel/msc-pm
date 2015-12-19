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
#include "interactionhandler.h"
#include "selectionhandler.h"
#include "effectivenessobserver.h"
#include "distortionobserver.h"
#include "npdistortion.h"
#include "skelft.h"

static QObject *mainProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return Main::instance();
}

static inline double hBeta(const arma::rowvec &Di, double beta, arma::rowvec &Pi) {
    Pi = arma::exp(-Di * beta);
    double sumPi = arma::accu(Pi);
    double h = log(sumPi) + beta * arma::accu(Di % Pi) / sumPi;
    Pi /= sumPi;
    return h;
}

static void calcP(const arma::mat &X, arma::mat &P, double perplexity = 30, double tol = 1e-5) {
    arma::colvec sumX = arma::sum(X % X, 1);
    arma::mat D = -2 * (X * X.t());
    D.each_col() += sumX;
    arma::inplace_trans(D);
    D.each_col() += sumX;
    D.diag() *= 0;
    double logU = log(perplexity);
    arma::rowvec beta(X.n_rows, arma::fill::ones);

    arma::rowvec Pi(X.n_rows);
    for (arma::uword i = 0; i < X.n_rows; i++) {
        double betaMin = -arma::datum::inf;
        double betaMax =  arma::datum::inf;
        arma::rowvec Di = D.row(i);
        double h = hBeta(Di, beta[i], Pi);

        double hDiff = h - logU;
        for (int tries = 0; fabs(hDiff) > tol && tries < 50; tries++) {
            if (hDiff > 0) {
                betaMin = beta[i];
                if (betaMax == arma::datum::inf || betaMax == -arma::datum::inf) {
                    beta[i] *= 2;
                } else {
                    beta[i] = (beta[i] + betaMax) / 2.;
                }
            } else {
                betaMax = beta[i];
                if (betaMin == arma::datum::inf || betaMin == -arma::datum::inf) {
                    beta[i] /= 2;
                } else {
                    beta[i] = (beta[i] + betaMin) / 2.;
                }
            }

            h = hBeta(Di, beta[i], Pi);
            hDiff = h - logU;
        }

        P.row(i) = Pi;
    }
}

arma::uvec relevanceSampling(const arma::mat &X, int subsampleSize)
{
    arma::mat P(X.n_rows, X.n_rows);
    calcP(X, P);
    P = (P + P.t());
    P /= arma::accu(P);
    P.transform([](double p) { return std::max(p, 1e-12); });

    arma::uvec indices = arma::sort_index(arma::sum(P));
    return indices(arma::span(0, subsampleSize - 1));
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("pm");
    app.setApplicationVersion("1.0");

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

    // Update splat whenever the main plot is also updated
    //QObject::connect(plot, SIGNAL(xyChanged(const arma::mat &)),
    //        splat, SLOT(setPoints(const arma::mat &)));
    //QObject::connect(plot, SIGNAL(colorDataChanged(const arma::vec &)),
    //        splat, SLOT(setValues(const arma::vec &)));

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
    //splat->setColorScale(&colorScale);
    plot->setColorData(labels);

    auto ret = app.exec();

    skelft2DDeinitialization();
    return ret;
}
