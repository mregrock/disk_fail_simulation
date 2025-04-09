#!/bin/bash

mkdir -p build/
cp data/ build/
cd build
cmake ..
make
