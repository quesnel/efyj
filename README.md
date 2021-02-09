# efyj

A tool to easily run evluation or optimization of
[DEXi](https://kt.ijs.si/MarkoBohanec/dexi.html) file (Multi-Attribute
Decision Making).

* Copyright © 2014-2021 INRA
* The software is released under the MIT license. See the COPYING file.

## Requirements

On Unix (Linux, OS X)

* libexpat (≥ 2)
* cmake (≥ 3.10)
* gcc ≥ 7.0 or clang ≥ 7.0
* python ≥ 3.0 (optional)
* R ≥ 4.0 (optional)

For recent Debian and Ubuntu derivatives (remove clang to only use gcc):

    apt-get install build-essential cmake clang libexpat1-dev

On Windows

* Visual Studio 2017 or newer
* cmake (≥ 3.10)
* [vcpkg](https://github.com/microsoft/vcpkg)
* [vcpkg expat](https://github.com/microsoft/vcpkg/tree/master/ports/expat)
* python ≥ 3.0 (optional)
* R ≥ 4.0 (optional)

    git clone https://github.com/microsoft/vcpkg
    .\vcpkg\bootstrap-vcpkg.bat
    .\vcpkg\vcpkg integrate install
    .\vcpkg\vcpkg install

## Python package installation

Clone this repository and pip install. Note the `--recursive` option
which is needed for the pybind11 and fmt submodule:

    git clone --recursive https://github.com/quesnel/efyj.git
    pip install ./efyj

## R package installation

Clone this repository and pip install. Note the `--recursive` option
which is needed for the pybind11 and fmt submodule:

    git clone --recursive https://github.com/quesnel/efyj.git

On Unix (Linux, OS X)

    cd efyj/refyj
    Rscript -e 'library(Rcpp); compileAttributes(".")'
    Rscript -e 'library(devtools); compileAttributes(".")'
    cd ..
    R CMD build refyj
    R CMD INSTALL --build refyj

On Windows

First we need to install the `expat` library for both i386
and x86_64 architecture. We use the *msys2* project to install this
dependencies. First install *msys2*:

[msys2 (x86_64)](https://repo.msys2.org/distrib/x86_64/msys2-x86_64-20210105.exe)

    pacman -Syu
    pacman -Su
    pacman -S expat libexpat-devel expat pkg-config cmake make
    pacman -S mingw32/mingw-w64-i686-expat
    pacman -S mingw64/mingw-w64-x86_64-expat
    pacman -S pkg-config cmake make

To build the R package. First under the R interface or Rstudio, we need to
install refyj dependencies:

    install.packages("stringi")
    install.packages("devtools")
    install.packages("roxygen2")
    install.packages("Rcpp")

And *rtools* and *R*

- [rtools 4.0](https://cran.r-project.org/bin/windows/base/R-4.0.3-win.exe)
- [R 4.0](https://cran.r-project.org/bin/windows/base/R-3.3.3-win.exe)

Then, each time you want to build the package:

    remove.packages("refyj")            # remove the old refyj package
                                        # if necessary.
    library(Rcpp)
    library(devtools)
    setwd("d:/natifvle/efyj/refyj")     # update the correct path
    Rcpp::compileAttributes(".")
    devtools::document()

Then in a terminal (`cmd.exe`):

    set PATH=C:\Program Files\R\R-3.3.2\bin;d:\msys64\usr\bin;C:\Rtools\bin
    set MINGW_PATH=d:/msys64
    d:
    cd natifvle/efyj
    R CMD build refyj
    R CMD INSTALL --build refyj

## Installation without python or R

Compiling and installing without Python support:

    git clone --recursive https://github.com/quesnel/efyj.git
    cd efyj
    mkdir build
    cd build
    cmake -DDisablePython=ON -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    make
    make install

## Command line usage

    efyj -m file.dxi -o file.csv [-p] [-a]

The `file.csv` contains the options of the model. Four first one
columns contains fields: site, fields, department, year. Next columns
are basic attributes. Finally, the last column contains the result of
the model dexi.

For example, if a DEXi file `test.dxi` have one aggregate attribute
`root` and three basic attributes `a`, `b` and `c`, the CSV file is:

	$ cat file.csv
	site;fields;department;year;a;b;c;observed
	xxxx;yyyy;1532;2010;ok;favourable;host;0-5
	...

The '-p' option starts a prediction algorithm, '-a' option starts am
adjustment algorithm.
