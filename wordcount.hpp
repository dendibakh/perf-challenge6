#ifndef WORDCOUNT_H
#define WORDCOUNT_H

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifdef SOLUTION
#include "as3richa/word_count.hpp"
#endif

// use it for init any object you may need.
void init();

struct WordCount {
  int count;
  std::string word;

  friend bool operator>(const WordCount &lh, const WordCount &rh) {
    if (lh.count > rh.count)
      return true;
    if (lh.count == rh.count)
      return lh.word < rh.word;
    return false;
  }

  friend bool operator!=(const WordCount &lh, const WordCount &rh) {
    return lh.count != rh.count || lh.word != rh.word;
  }

#ifdef SOLUTION
  friend bool operator!=(const WordCount &lh, const WordCountEntry &rh) {
    return lh.count != rh.count || lh.word != rh.word;
  }
#endif
};

#ifndef SOLUTION
std::vector<WordCount> wordcount(std::string fileName);
#endif

#endif