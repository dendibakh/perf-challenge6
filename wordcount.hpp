#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

// use it for init any object you may need.
void init();

#ifdef SOLUTION
#include <memory_resource>

using FastString = std::basic_string<char, std::char_traits<char>,
  std::pmr::polymorphic_allocator<char>>;

struct WordCount {
  uint32_t distance = 0;
  int count = 0;
  uint64_t hash = 0;
  FastString word;

  WordCount() = default;
  ~WordCount() = default;

  WordCount(const WordCount&) = default;
  WordCount(WordCount&&) noexcept = default;

  WordCount& operator=(WordCount other) noexcept
  {
    std::swap(distance, other.distance);
    std::swap(count, other.count);
    std::swap(hash, other.hash);
    std::swap(word, other.word);
    return *this;
  }

  WordCount(int count_, std::string&& word_)
    : count(count_)
    , word(word_.data(), word_.size())
  {
  }

  void emplace(uint32_t distance_, int count_, uint64_t hash_, FastString&& key)
  {
    distance = distance_;
    count = count_;
    hash = hash_;
    word = std::move(key);
  }

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
#else
struct WordCount {
  int count;
  std::string word;

  friend bool operator>(const WordCount& lh, const WordCount& rh) {
    if (lh.count > rh.count)
      return true;
    if (lh.count == rh.count)
      return lh.word < rh.word;
    return false;
  }

  friend bool operator!=(const WordCount& lh, const WordCount& rh) {
    return lh.count != rh.count || lh.word != rh.word;
  }
};
#endif

std::vector<WordCount> wordcount(std::string fileName);
