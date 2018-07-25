#!/bin/bash

set -ex

cd EASTL

mkdir -p build/Release
cd build/Release
cmake -DCMAKE_BUILD_TYPE=Release ../..
cmake --build . --config Release

cd ../..

mkdir -p build/Debug
cd build/Debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build . --config Debug

cd ../../
