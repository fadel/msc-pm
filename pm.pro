QT += qml quick widgets

CONFIG += qt debug

win32 {
    ARMADILLO_DIR = "C:\Armadillo"

    QMAKE_LIBDIR += "$$ARMADILLO_DIR\lib"
    DEFINES += ARMA_USE_LAPACK ARMA_USE_BLAS
    INCLUDEPATH += "$$ARMADILLO_DIR\include"
    LIBS += -llapack_win64_MT -lblas_win64_MT
}

msvc:QMAKE_CXXFLAGS += /openmp

unix {
    QMAKE_CXXFLAGS += -std=c++11 -fopenmp
    LIBS += -larmadillo -fopenmp
}

HEADERS += main.h \
    colorscale.h \
    continuouscolorscale.h \
    divergentcolorscale.h \
    geometry.h \
    scale.h \
    scatterplot.h \
    voronoisplat.h \
    colormap.h \
    historygraph.h \
    barchart.h \
    transitioncontrol.h \
    transitionworkerthread.h \
    manipulationhandler.h \
    mapscalehandler.h \
    numericrange.h \
    selectionhandler.h \
    brushinghandler.h \
    projectionhistory.h \
    skelft.h \
    skelftkernel.h \
    mp.h

SOURCES += main.cpp \
    colorscale.cpp \
    continuouscolorscale.cpp \
    divergentcolorscale.cpp \
    geometry.cpp \
    scatterplot.cpp \
    voronoisplat.cpp \
    colormap.cpp \
    historygraph.cpp \
    barchart.cpp \
    transitioncontrol.cpp \
    transitionworkerthread.cpp \
    manipulationhandler.cpp \
    mapscalehandler.cpp \
    selectionhandler.cpp \
    brushinghandler.cpp \
    projectionhistory.cpp \
    skelft_core.cpp \
    lamp.cpp \
    plmp.cpp \
    knn.cpp \
    forcescheme.cpp \
    measures.cpp \
    dist.cpp

OTHER_FILES += skelft.cu

# CUDA settings
CUDA_SOURCES += skelft.cu
CUDA_ARCH = sm_30
NVCC_LIBS = -lcuda -lcudart
NVCC_OPTIONS += --use_fast_math

win32 {
    CUDA_DIR = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v7.5"
    CUDA_INCLUDEPATH += $$shell_quote($$CUDA_DIR\include)
    NVCC = $$CUDA_DIR\bin\nvcc
    QMAKE_LIBDIR += "$$CUDA_DIR\lib\x64"
    LIBS += cuda.lib cudart.lib
}

unix {
    CUDA_DIR = "/opt/cuda"
    CUDA_INCLUDEPATH += $$CUDA_DIR/include
    NVCC = $$CUDA_DIR/bin/nvcc
    QMAKE_LIBDIR += $$CUDA_DIR/lib64
    LIBS += $$NVCC_LIBS
}

#macx {
#    CUDA_DIR = "/Developer/GPU Computing"
#    CUDA_INCLUDEPATH += $$shell_quote($$CUDA_DIR/shared/inc)
#    CUDA_INCLUDEPATH += $$shell_quote($$CUDA_DIR/C/common/inc)
#    NVCC = /usr/local/cuda/bin/nvcc
#    QMAKE_LIBDIR += $$CUDA_DIR/CUDALibraries/common/lib
#    LIBS += $$NVCC_LIBS
#}

NVCC_INCS = $$join(CUDA_INCLUDEPATH, " -I", "-I", "")
SYSTEM_TYPE = 64

CONFIG(debug, debug|release) {
    # msvc: use /MDd, just as Qt does
    msvc:NVCC_OPTIONS += -Xcompiler "/MDd"

    cuda_dbg.input = CUDA_SOURCES
    cuda_dbg.output = ${QMAKE_FILE_BASE}_cuda.o
    cuda_dbg.commands = $$NVCC -D_DEBUG -g $$NVCC_OPTIONS $$NVCC_INCS $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda_dbg.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda_dbg
} else {
    # msvc: use /MD, just as Qt does
    msvc:NVCC_OPTIONS += -Xcompiler "/MD"

    cuda.input = CUDA_SOURCES
    cuda.output = ${QMAKE_FILE_BASE}_cuda.o
    cuda.commands = $NVCC $$NVCC_OPTIONS $$NVCC_INCS $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda
}

RESOURCES += pm.qrc

target.path = .
INSTALLS += target
