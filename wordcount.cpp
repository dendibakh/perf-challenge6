#include "wordcount.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

#include <string_view>
#ifndef _WIN32
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <fcntl.h>
#endif

// Assumptions
// 1. Function should read the input from the file, i.e. caching the input is
// not allowed.
// 2. The input is always encoded in UTF-8.
// 3. Break only on space, tab and newline (do not break on non-breaking space).
// 4. Sort words by frequency AND secondary sort in alphabetical order.

// Implementation rules
// 1. You can add new files but dependencies are generally not allowed unless it
// is a header-only library.
// 2. Your submission must be single-threaded, however feel free to implement
// multi-threaded version (optional).

#ifdef SOLUTION
using phmap::flat_hash_map;

std::vector<WordCount> wordcount(std::string filePath) {
  flat_hash_map<std::string, int> m;

  std::vector<WordCount> mvec;
#ifndef _WIN32
  int fd = open(filePath.c_str(), O_RDONLY);
  if (fd<0)
    std::cerr << "file open error\n";

  struct stat st{};
  if (fstat(fd, &st) == -1)
    std::cerr << "file size error\n";

  size_t fs = static_cast<std::size_t>(st.st_size);

  const char* mm = static_cast<const char*>(mmap(nullptr, fs, PROT_READ, MAP_PRIVATE, fd, 0U));

  std::string_view vs = std::string_view{mm, static_cast<std::size_t>(fs)};

  const char* begin = vs.begin();
  const char* curr  = begin;
  const char* end   = vs.end();

  while (curr != end) {
    if (*curr==' ' || *curr=='\n' || *curr=='\t') {
      std::string s = std::string{&*begin, static_cast<std::size_t>(curr - begin)};
      if (!isspace(s[0]) && !s.empty()) {
      	m[s]++;
      }
      begin = std::next(curr);
    }
    std::advance(curr, 1);
  }

  std::string s = std::string{&*begin, static_cast<std::size_t>(curr - begin)};
  if (!isspace(s[0]) && !s.empty()) {
    m[s]++;
  }
#else // _WIN32
  std::ifstream inFile{filePath};
  if (!inFile) {
    std::cerr << "Invalid input file: " << filePath << "\n";
    return mvec;
  }

  std::string s;
  while (inFile >> s)
    m[s]++;
#endif // _WIN32

  mvec.reserve(m.size());
  for (auto &p : m)
    mvec.emplace_back(WordCount{p.second, move(p.first)});

  std::sort(mvec.begin(), mvec.end(), std::greater<WordCount>());
#ifndef _WIN32  
  munmap(static_cast<void*>(const_cast<char*>(mm)), fs);
#endif  
  return mvec;
}

#else
// Baseline solution.
// Do not change it - you can use for quickly checking speedups
// of your solution agains the baseline, see check_speedup.py
std::vector<WordCount> wordcount(std::string filePath) {
  std::unordered_map<std::string, int> m;
  m.max_load_factor(0.5);

  std::vector<WordCount> mvec;

  std::ifstream inFile{filePath};
  if (!inFile) {
    std::cerr << "Invalid input file: " << filePath << "\n";
    return mvec;
  }

  std::string s;
  while (inFile >> s)
    m[s]++;

  mvec.reserve(m.size());
  for (auto &p : m)
    mvec.emplace_back(WordCount{p.second, move(p.first)});

  std::sort(mvec.begin(), mvec.end(), std::greater<WordCount>());
  return mvec;
}
#endif
