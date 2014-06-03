efyj
====

Copyright © 2014 INRA

The software is released under the MIT license. See the COPYING file.

## Requirements

* libexpat (≥ 2.0)
* boost (≥ 1.50)
* cmake (≥ 2.8.0)
* make (≥ 1.8)
* c++ compiler (gcc ≥ 4.8, clang ≥ 3.3, intel icc (≥ 11.0).

For recent Debian and Ubuntu derivatives (remove clang to only use gcc):

    apt-get install build-essential cmake clang libexpat1-dev \
                    libboost-dev

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

* Not yet usable.
