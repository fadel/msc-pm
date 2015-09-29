QT += qml quick widgets

QMAKE_CXXFLAGS += -std=c++11 -fopenmp
QMAKE_LIBS += -larmadillo -fopenmp
HEADERS += colorscale.h \
    continuouscolorscale.h \
    geometry.h \
    scale.h \
    scatterplot.h \
    historygraph.h \
    interactionhandler.h \
    selectionhandler.h \
    effectivenessobserver.h \
    distortionobserver.h \
    distortionmeasure.h \
    npdistortion.h \
    mp.h
SOURCES += main.cpp \
    colorscale.cpp \
    continuouscolorscale.cpp \
    geometry.cpp \
    scatterplot.cpp \
    historygraph.cpp \
    interactionhandler.cpp \
    selectionhandler.cpp \
    effectivenessobserver.cpp \
    distortionobserver.cpp \
    npdistortion.cpp \
    lamp.cpp \
    forceScheme.cpp \
    tsne.cpp \
    measures.cpp \
    dist.cpp
RESOURCES += pm.qrc

target.path = .
INSTALLS += target
