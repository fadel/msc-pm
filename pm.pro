QT += qml quick

QMAKE_LIBS += -larmadillo
HEADERS += scatterplot.h \
    mp.h
SOURCES += main.cpp \
    scatterplot.cpp \
    lamp.cpp \
    forceScheme.cpp \
    dist.cpp
RESOURCES += pm.qrc

target.path = .
INSTALLS += target
