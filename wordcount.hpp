#ifndef WORD_COUNT_H
#define WORD_COUNT_H
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

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
};
std::vector<WordCount> wordcount(std::string fileName);

#endif /* WORD_COUNT_H */