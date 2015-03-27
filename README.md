efyj
====

Copyright © 2014-2015 INRA

The software is released under the MIT license. See the COPYING file.

## Requirements

* libexpat (≥ 2)
* boost (≥ 1.50)
* eigen3 (≥ 3)
* cmake (≥ 2.8)
* make (≥ 1.8)
* c++ compiler (gcc ≥ 4.8, clang ≥ 3.3, intel icc (≥ 11.0).

For recent Debian and Ubuntu derivatives (remove clang to only use gcc):

    apt-get install build-essential cmake clang libexpat1-dev \
                    libboost-dev libeigen3-dev

## Compilation

Compiling and installing:

    cd efyj
    mkdir build
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
    make -j
    make install

To use clang replace the previous `cmake` command:

    CXX=clang++-libc++ cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..

# Usage

    $ efyj -m file.dxi -o file.csv

The `file.csv` contains the options of the model. Four first one columns contains fields: site, fields, department, year. Next columns are basic attributes. Finally, the last column contains the result of the model dexi.

For example, if a DEXi file `test.dxi` have one aggregate attribute `root` and three basic attributes `a`, `b` and `c`, the CSV file is:

	$ cat file.csv
	site;fields;departement;year;a;b;c;result
	xxxx;yyyy;1532;2010;ok;favourable;host;0-5
	...