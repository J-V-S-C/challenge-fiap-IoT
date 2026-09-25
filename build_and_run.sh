#!/bin/bash

echo "Building..."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --clean-first

echo "Starting..."

./build/clinical_simulator
