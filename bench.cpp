
#include "benchmark/benchmark.h"
#include "wordcount.hpp"
#include <iostream>

static std::string inputsDirPath;

static void bench1(benchmark::State &state) {
  // Init benchmark data
  // auto arg = ...;
  init(/*arg*/);

  // Run the benchmark
  for (auto _ : state) {
    auto output = wordcount(inputsDirPath);
    benchmark::DoNotOptimize(output);
  }
}

// Register the function as a benchmark and measure time in microseconds
BENCHMARK(bench1)->Unit(benchmark::kSecond);

// Run the benchmark
int main(int argc, char **argv) {
  constexpr int mandatoryArgumentsCount = 1;
  if (argc < 1 + mandatoryArgumentsCount) {
    std::cerr << "Usage: lab path/to/inputs [gbench args]" << std::endl;
    return 1;
  }
  inputsDirPath = argv[1];

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc - mandatoryArgumentsCount,
                                               argv + mandatoryArgumentsCount))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();
  return 0;
}
