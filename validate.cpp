
#include "wordcount.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::vector<WordCount> parseRefOut(const std::string &refFilePath) {
  std::vector<WordCount> mvec;
  std::ifstream inFile{refFilePath};
  if (!inFile) {
    std::cerr << "Invalid output file: " << refFilePath << "\n";
    return mvec;
  }

  std::string s;
  int count;
  while (inFile >> s >> count)
    mvec.emplace_back(WordCount{count, move(s)});

  return mvec;
}

bool validateOneInputFile(const fs::path &filePath, const fs::path &refPath) {
  std::string testFileName = filePath.stem().string();
  fs::path outRefFilePath = refPath;
  outRefFilePath.append(testFileName + ".out");
  if (!fs::exists(outRefFilePath)) {
    std::cout << testFileName << " : FAIL"
              << " - no reference output file." << std::endl;
    return false;
  }

  auto refResult = parseRefOut(outRefFilePath);
  auto result = wordcount(filePath);

  // dump result for post-checking
  std::string outResFilePath = testFileName + ".out";
  std::ofstream outFile{outResFilePath};
  for (auto wc : result) {
    outFile << wc.word << "\t" << wc.count << '\n';
  }

  if (refResult.size() != result.size()) {
    std::cout << testFileName << " : FAIL" << std::endl;
    std::cerr << "  Number of words is different. ref = " << refResult.size()
              << " ; res = " << result.size() << "\n";
    std::cerr << "  Run 'diff -u " << outResFilePath << " "
              << outRefFilePath.string() << "' to see the difference.\n";
    return false;
  }

  int num_errors = 0;
  for (int i = 0; i < refResult.size(); i++) {
    if (refResult[i] != result[i]) {
      if (num_errors == 0)
        std::cout << testFileName << " : FAIL" << std::endl;
      std::cerr << "  Validation failed for line#: " << i << "\n"
                << "    reference line  = " << refResult[i].word << " "
                << refResult[i].count << "\n"
                << "    solution output = " << result[i].word << " "
                << result[i].count << "\n";
      num_errors++;
    }
    // stop after first 5 errors.
    if (num_errors > 5) {
      return false;
    }
  }

  if (num_errors > 0)
    return false;

  std::cout << testFileName << " : PASS" << std::endl;
  return true;
}

int main(int argc, char **argv) {
  constexpr int mandatoryArgumentsCount = 2;
  if (argc != 1 + mandatoryArgumentsCount) {
    std::cerr << "Usage: lab path/to/inputs path/to/ref" << std::endl;
    return 1;
  }

  const fs::path inputsDirPath = argv[1];
  if (!fs::exists(inputsDirPath)) {
    std::cerr << "Validation Failed. Bad inputs path." << std::endl;
    return 1;
  }

  const fs::path refDirPath = argv[2];
  if (!fs::exists(refDirPath) || !fs::is_directory(refDirPath)) {
    std::cerr << "Validation Failed. Bad ref dir." << std::endl;
    return 1;
  }

  if (fs::is_regular_file(inputsDirPath))
    return validateOneInputFile(inputsDirPath, refDirPath) ? 0 : 1;

  if (!fs::is_directory(inputsDirPath)) {
    std::cerr << "Validation Failed. Bad input file." << std::endl;
    return 1;
  }

  bool validSolution = true;
  for (const auto &dirEntry : fs::recursive_directory_iterator(inputsDirPath)) {
    if (fs::is_regular_file(dirEntry))
      validSolution &= validateOneInputFile(dirEntry.path(), refDirPath);
  }

  if (validSolution) {
    std::cout << "ALL TESTS PASSED" << std::endl;
    return 0;
  }
  return 1;
}
