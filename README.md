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
* python ≥ 3.0

For recent Debian and Ubuntu derivatives (remove clang to only use gcc):

    apt-get install build-essential cmake clang libexpat1-dev

On Windows

* Visual Studio 2017 or newer
* cmake (≥ 3.10)
* [vcpkg](https://github.com/microsoft/vcpkg)
* [vcpkg expat](https://github.com/microsoft/vcpkg/tree/master/ports/expat)
* python ≥ 3.0

    git clone https://github.com/microsoft/vcpkg
    .\vcpkg\bootstrap-vcpkg.bat
    .\vcpkg\vcpkg integrate install
    .\vcpkg\vcpkg install

## Installation via python

Clone this repository and pip install. Note the `--recursive` option
which is needed for the pybind11 and fmt submodule:

    git clone --recursive https://github.com/quesnel/efyj.git
    pip install ./efyj

## Installation without python

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
