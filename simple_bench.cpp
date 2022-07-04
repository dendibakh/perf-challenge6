#include "wordcount.hpp"
#include <iostream>

#define SOLUTION

static std::string inputsDirPath;

int main(int argc, char **argv) {
  constexpr int mandatoryArgumentsCount = 1;
  if (argc < 1 + mandatoryArgumentsCount) {
    std::cerr << "Usage: lab path/to/inputs [gbench args]" << std::endl;
    return 1;
  }
  inputsDirPath = argv[1];
  wordcount(inputsDirPath);
  return 0;
}
