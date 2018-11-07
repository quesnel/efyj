#!/bin/bash

set -ex

cd EASTL

mkdir -p out/Release
cd out/Release
cmake -DCMAKE_BUILD_TYPE=Release ../..
cmake --build . --config Release

cd ../..

mkdir -p out/Debug
cd out/Debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build . --config Debug

cd ../../
