cmake_minimum_required (VERSION 2.8.12)

project (pm)

if ((${CMAKE_CXX_COMPILER_ID} MATCHES "GNU") OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
    set(CMAKE_CXX_FLAGS "-std=c++11")
endif()

# Current dir should be included in search path, right? :)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run moc on Qt5 (sub-) classes
set(CMAKE_AUTOMOC ON)

# http://arma.sourceforge.net/
find_package(Armadillo)

# Will probably work with most CUDA releases
find_package(CUDA)

# Qt5 packages
find_package(Qt5 COMPONENTS Core Qml Quick Widgets)

# The application can significantly speed up some operations using multiple
# threads via OpenMP, if supported
find_package(OpenMP)
if (OPENMP_FOUND)
    #set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# This is the only one tested, might work with other configs
set(CUDA_NVCC_FLAGS -arch=compute_30)

qt5_add_resources(RESOURCES
    pm.qrc)

cuda_add_executable(pm
    main.cpp
    barchart.cpp
    brushinghandler.cpp
    colormap.cpp
    colorscale.cpp
    continuouscolorscale.cpp
    dist.cpp
    divergentcolorscale.cpp
    forcescheme.cpp
    geometry.cpp
    historygraph.cpp
    knn.cpp
    lamp.cpp
    manipulationhandler.cpp
    mapscalehandler.cpp
    measures.cpp
    plmp.cpp
    projectionhistory.cpp
    scatterplot.cpp
    selectionhandler.cpp
    skelft.cu
    skelft_core.cpp
    transitioncontrol.cpp
    transitionworkerthread.cpp
    voronoisplat.cpp
    ${RESOURCES})

target_link_libraries(pm
    ${ARMADILLO_LIBRARIES}
    Qt5::Widgets
    Qt5::Qml
    Qt5::Quick)

install(TARGETS pm RUNTIME DESTINATION ${CMAKE_SOURCE_DIR})
