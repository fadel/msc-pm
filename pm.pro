QT += qml quick widgets

CONFIG += qt debug

QMAKE_CXXFLAGS += -std=c++11 -fopenmp
QMAKE_LIBS += -larmadillo -fopenmp
HEADERS += main.h \
    colorscale.h \
    continuouscolorscale.h \
    geometry.h \
    scale.h \
    scatterplot.h \
    voronoisplat.h \
    historygraph.h \
    barchart.h \
    interactionhandler.h \
    selectionhandler.h \
    effectivenessobserver.h \
    distortionobserver.h \
    distortionmeasure.h \
    npdistortion.h \
    skelft.h \
    skelftkernel.h \
    mp.h
SOURCES += main.cpp \
    colorscale.cpp \
    continuouscolorscale.cpp \
    geometry.cpp \
    scatterplot.cpp \
    voronoisplat.cpp \
    historygraph.cpp \
    barchart.cpp \
    interactionhandler.cpp \
    selectionhandler.cpp \
    effectivenessobserver.cpp \
    distortionobserver.cpp \
    npdistortion.cpp \
    skelft_core.cpp \
    lamp.cpp \
    plmp.cpp \
    knn.cpp \
    forceScheme.cpp \
    tsne.cpp \
    measures.cpp \
    dist.cpp

OTHER_FILES += skelft.cu

# Cuda settings
CUDA_SOURCES += skelft.cu
CUDA_DIR = "/opt/cuda"
CUDA_ARCH = sm_30
NVCC_OPTIONS += --use_fast_math
SYSTEM_TYPE = 64  # Either 64 or empty
INCLUDEPATH += $$CUDA_DIR/include
QMAKE_LIBDIR += $$CUDA_DIR/lib$$SYSTEM_TYPE
LIBS += -lcuda -lcudart

CONFIG(debug, debug|release) {
    cuda_dbg.input = CUDA_SOURCES
    cuda_dbg.output = ${QMAKE_FILE_BASE}_cuda.o
    cuda_dbg.commands = $$CUDA_DIR/bin/nvcc -D_DEBUG -g $$NVCC_OPTIONS -I$$INCLUDEPATH $$LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda_dbg.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda_dbg
} else {
    cuda.input = CUDA_SOURCES
    cuda.output = ${QMAKE_FILE_BASE}_cuda.o
    cuda.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS -I$$INCLUDEPATH $$LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda
}

RESOURCES += pm.qrc

target.path = .
INSTALLS += target
