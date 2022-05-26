cmake_minimum_required(VERSION 2.8.12)

project(wordcount)

# 1. CHECK YOUR SOLUTION ON SMALL TEST INPUTS
# set(VALIDATE_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/data/test")

# 2. RUN QUICK EXPERIMENTS ON SMALL INPUT FILE
# set(BENCHMARK_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/data/small.data")

# 3. FOR SUBMISSION USE LARGE INPUT FILE FOR BENCHMARKING
set(BENCHMARK_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/data/large.data")
set(VALIDATE_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/data/small.data")

# We do not validate your code on large data file,
# but you can do it if you want. Feel free to generate large.out
# using the baseline code and put it in 'ref' directory.

# Add second argument with path to reference outputs
set(VALIDATE_ARGS ${VALIDATE_ARGS} "${CMAKE_CURRENT_SOURCE_DIR}/ref")

include(cmake/helper.cmake)
