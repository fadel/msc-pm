QT += qml quick widgets

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LIBS += -larmadillo
HEADERS += colorscale.h \
    scatterplot.h \
    interactionhandler.h \
    mp.h
SOURCES += main.cpp \
    colorscale.cpp \
    scatterplot.cpp \
    interactionhandler.cpp \
    lamp.cpp \
    forceScheme.cpp \
    dist.cpp
RESOURCES += pm.qrc

target.path = .
INSTALLS += target
