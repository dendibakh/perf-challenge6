#include <cassert>
#include <iostream>

#define NDEBUG

#include "word_count.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: " << std::string_view(argv[0])
              << " <input filename>\n";
    return 1;
  }

  WordCountResult result = wordcount(std::string(argv[1]));

  for (WordCountEntry wc : result) {
    std::cout << wc.word << ' ' << wc.count << '\n';
  }

  return 0;
}