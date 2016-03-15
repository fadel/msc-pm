Interactive multidimensional projections.

# Prerequisites
This program depends on the following libraries:

* [CUBu](https://bitbucket.org/rmmartins/cubu) (download the sources and follow
  the instructions on README.md to build and install; you *must* edit this
  project's CMakeLists.txt and point the CUBU variable to the base folder)
* Qt 5 (Qt5Widgets, Qt5Qml, Qt5Quick)
* Armadillo 6.x.x (might work with older versions)
* CUDA 7.5 (high chance of working with older versions)
* CMake 2.8.12 (or newer)

CMake *should* automatically find all the mentioned libraries under normal
circumstances (not Windows).

## Windows
On Windows, Armadillo should be installed in `C:\Program Files (x86)\Armadillo`
(this is where CMake usually looks for Armadillo). Also remember to build
`armadillo.lib` instead of the default `armadillo.dll`.

# Building
Assuming the current directory is the root of this project:

    mkdir build
    cd build
    cmake ..
    make

For faster builds, you can use `make -j NUM_CORES`.

# Usage
This program needs a CUDA-enabled GPU (not necessarily powerful) to run. The
program is run as follows:

    ./pm [options] dataset

Options accepted are:

Option                   | Description
-------------------------| --------------------------------------------------------------------------------------------
-h, --help               | Displays usage.
-v, --version            | Displays version information.
-i, --indices <filename> | Filename to store the control points' indices. Omitting this option disables saving indices.
-c, --cpoints <filename> | Filename to store the control points' map. Omitting this option disables saving this map.

And the arguments are:

Argument | Description
---------|-----------------------------
dataset  | Dataset filename (.tbl file)

# File formats
An **indices file** should be a file where each line contains an index (starting
from zero) and nothing else. Each index will be considered a control point, in
the context of the given dataset file.

A **CP map file** should be a two-column file where each line contains the 2D
coordinates of each control point (according to the indices file) and nothing
else. Note that the number of lines of this file and the indices file are
supposed to be the same.

**Dataset files** are the same as CP map files, except they are allowed to have
any number of columns. In addition, the last column is assumed to be the class
labels (currently unused, but must be present). Node that the number of columns
must be the same on each line.
