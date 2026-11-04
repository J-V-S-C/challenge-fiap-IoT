#!/bin/bash
set -euo pipefail

echo "Building..."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

echo "Starting..."

exec ./build/clinical_simulator
