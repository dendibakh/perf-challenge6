#!/usr/bin/env bash

set -euxo pipefail

clang++ -o wordcount -O3 main.cpp -std=c++17 -march=native
