#include <cstdint>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

// use it for init any object you may need.
void init();

// clang-format off
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
// clang-format on

#ifdef SOLUTION

class StringView {
public:
  StringView() = default;

  StringView(const char* data, uint32_t length, uint32_t hash)
      : data_(data)
      , length_(length)
      , hash_(hash) {}

  friend bool operator==(StringView a, StringView b) {
    if (a.length_ != b.length_) {
      return false;
    }
    for (uint32_t i = 0; i < a.length_ / 4; i++) {
      if (*((uint32_t*)a.data_ + i) != *((uint32_t*)b.data_ + i)) {
        return false;
      }
    }
    for (uint32_t i = (a.length_ / 4) * 4; i < a.length_; i++) {
      if (a.data_[i] != b.data_[i]) {
        return false;
      }
    }
    return true;
  }
  friend bool operator!=(StringView a, StringView b) {
    return !(a == b);
  }
  friend bool operator<(StringView a, StringView b) {
    uint32_t len = std::min(a.length_, b.length_);
    for (uint32_t i = 0; i <= len; i++) {
      if (a.data_[i] != b.data_[i]) {
        return (unsigned char)a.data_[i] < (unsigned char)b.data_[i];
      }
    }
    return false;
  }

  uint32_t length() const {
    return length_;
  }
  char operator[](ptrdiff_t index) const {
    return data_[index];
  }
  const char* begin() const {
    return data_;
  }
  const char* end() const {
    return begin() + length();
  }
  uint32_t hash() const {
    return hash_;
  }

private:
  const char* data_ = nullptr;
  uint32_t length_ = 0;
  uint32_t hash_ = 0;
};

inline std::ostream& operator<<(std::ostream& out, StringView view) {
  out << view.begin();
  return out;
}

static_assert(sizeof(StringView) == 16);

constexpr size_t kArenaDefaultBufferSize = 256 * 1024 * 1024;

class Arena {
public:
  Arena() {
    Grow(kArenaDefaultBufferSize);
  }

  StringView Allocate(StringView word) {
    if (tail_ + word.length() + 1 >= buffer_->size()) {
      Grow(std::max(kArenaDefaultBufferSize, (size_t)word.length() + 1));
    }
    std::copy(word.begin(), word.end(), &buffer_->at(tail_));
    tail_ += word.length() + 1;
    return StringView(&buffer_->at(tail_ - word.length() - 1), word.length(),
                      word.hash());
  }

private:
  void Grow(size_t buffer_size) {
    buffers_.push_back(std::vector<char>(buffer_size));
    buffer_ = &buffers_.back();
    tail_ = 0;
  }
  std::list<std::vector<char>> buffers_;
  std::vector<char>* buffer_;
  size_t tail_;
};

struct Node {
  StringView word;
  int count;
  friend bool operator<(const Node& a, const Node& b) {
    if (a.count != b.count) {
      return a.count > b.count;
    }
    return a.word < b.word;
  }
};

inline bool operator==(const WordCount& wc, const Node& node) {
  return wc.count == node.count && wc.word == node.word.begin();
}

inline bool operator!=(const WordCount& wc, const Node& node) {
  return !(wc == node);
}

class WordStat {
public:
  WordStat(Arena arena, std::vector<Node> nodes)
      : arena_(std::move(arena))
      , nodes_(std::move(nodes)) {}

  const Node& operator[](ptrdiff_t index) const {
    return nodes_[index];
  }

  auto begin() {
    return nodes_.begin();
  }
  auto end() {
    return nodes_.end();
  }

  auto begin() const {
    return nodes_.begin();
  }
  auto end() const {
    return nodes_.end();
  }

  std::size_t size() const {
    return nodes_.size();
  }

private:
  Arena arena_;
  std::vector<Node> nodes_;
};

WordStat wordcount(const std::string& fileName);

#else

std::vector<WordCount> wordcount(std::string fileName);

#endif