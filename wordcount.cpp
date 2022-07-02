#include "wordcount.hpp"

#include <algorithm>
#include <fstream>

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

namespace {

#define CHECK(condition)                                                      \
  if (!(condition)) {                                                         \
    std::cerr << "A check `" << #condition << "` failed in line " << __LINE__ \
              << "\n";                                                        \
    exit(EXIT_FAILURE);                                                       \
  }

#ifdef _WIN32

#include <windows.h>

#undef min
#undef max

class FileMapping {
public:
  FileMapping(const std::string& filename) {
    file_ = CreateFileA(filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(file_ != NULL);

    DWORD hfs;
    DWORD lfs = GetFileSize(file_, &hfs);
    file_size_ = ((int64_t)hfs << 32) + lfs;

    mapping_ = CreateFileMappingA(file_, NULL, PAGE_READONLY, 0, 0, NULL);
    CHECK(mapping_ != NULL);

    data_ = (char*)MapViewOfFile(mapping_, FILE_MAP_READ, 0, 0, 0);
    CHECK(data_ != NULL);
  }
  ~FileMapping() {
    CHECK(UnmapViewOfFile(data_));
    CHECK(CloseHandle(mapping_));
    CHECK(CloseHandle(file_));
  }

  const char* data() const {
    return data_;
  }
  int64_t size() const {
    return file_size_;
  }

private:
  HANDLE file_;
  int64_t file_size_;
  HANDLE mapping_;
  char* data_;
};

#else

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class FileMapping {
public:
  FileMapping(const std::string& filename) {
    struct stat statbuf;
    file_ = open(filename.c_str(), O_RDONLY, 0);
    CHECK(file_ != -1);
    fstat(file_, &statbuf);
    file_size_ = statbuf.st_size;

    data_ = (char*)mmap(NULL, file_size_, PROT_READ, MAP_SHARED, file_, 0);
    CHECK(data_ != MAP_FAILED);
  }

  ~FileMapping() {
    CHECK(munmap(data_, file_size_) == 0);
    CHECK(close(file_) == 0);
  }

  const char* data() const {
    return data_;
  }
  int64_t size() const {
    return file_size_;
  }

private:
  int file_;
  int64_t file_size_;
  char* data_;
};

#endif

template <typename Vector>
void RemoveNoHits(Vector& v) {
  auto zit = std::remove_if(v.begin(), v.end(), [](const auto& a) {
    return a.count == 0;
  });
  v.resize(zit - v.begin());
}

class ShortWordCounter {
public:
  ShortWordCounter(double max_load_factor) {
    max_load_factor_ = max_load_factor;
    capacity_ = 1 * 1024 * 1024;
    nodes_.resize(capacity_);
    occupied_ = 0;
  }

  void Increment(const char* data, uint8_t length, uint32_t hash) {
    ShortStringNode* node = FindNode((const uint8_t*)data, length, hash);
    if (++node->count > 1) {
      return;
    }
    for (int i = 0; i < length; i++) {
      node->c_str[i] = data[i];
    }
    node->c_str[length] = 0;
    node->length = length;
    node->hash = hash;
    ++occupied_;
  }

  static const size_t kShortStringSize = 22;
  struct ShortStringNode {
    uint8_t c_str[kShortStringSize + 1];
    uint8_t length = 0;
    uint32_t hash;
    int count = 0;

    friend bool operator<(const ShortStringNode& a, const ShortStringNode& b) {
      if (a.count != b.count) {
        return a.count > b.count;
      }
      for (size_t i = 0; i <= kShortStringSize; i++) {
        if (a.c_str[i] != b.c_str[i]) {
          return a.c_str[i] < b.c_str[i];
        }
      }
      // will never get here
      return false;
    }
    bool operator==(const uint8_t* data) const {
      for (int i = 0; i < length; i++) {
        if ((uint8_t)data[i] != c_str[i]) {
          return false;
        }
      }
      return true;
    }
  };

  std::vector<ShortStringNode> SortedResult() {
    RemoveNoHits(nodes_);
    std::sort(nodes_.begin(), nodes_.end());
    return std::move(nodes_);
  }

  void MaybeRehash() {
    if (occupied_ <= capacity_ * max_load_factor_) {
      return;
    }
    capacity_ *= 4;
    std::vector<ShortStringNode> old_nodes(capacity_);
    nodes_.swap(old_nodes);
    for (ShortStringNode& old_node : old_nodes) {
      if (old_node.count > 0) {
        *FindNode(old_node.c_str, old_node.length, old_node.hash) = old_node;
      }
    }
  }

private:
  ShortStringNode* FindNode(const uint8_t* data, uint8_t length,
                            uint32_t hash) {
    size_t node_id = hash & (capacity_ - 1);
    for (;;) {
      if (nodes_[node_id].count == 0) {
        return &nodes_[node_id];
      }
      if (nodes_[node_id].length == length && nodes_[node_id] == data) {
        return &nodes_[node_id];
      }
      node_id = (node_id + 1) & (capacity_ - 1);
    }
  }

  std::vector<ShortStringNode> nodes_;
  size_t capacity_;
  size_t occupied_;
  double max_load_factor_;
};

class LongWordCounter {
public:
  LongWordCounter(double max_load_factor) {
    max_load_factor_ = max_load_factor;
    capacity_ = 1 * 1024 * 1024;
    nodes_.resize(capacity_);
    occupied_ = 0;
  }

  void Increment(StringView word) {
    Node* node = FindNode(word);
    if (++node->count > 1) {
      return;
    }
    node->word = arena_.Allocate(word);
    ++occupied_;
  }

  WordStat Result(ShortWordCounter& shorts) {
    RemoveNoHits(nodes_);
    CHECK(nodes_.size() == occupied_);
    auto mid = std::partition(nodes_.begin(), nodes_.end(), [](const Node& a) {
      return a.count > 1;
    });

    std::vector<std::vector<StringView>> buckets(256 * 256);
    auto it = mid;
    while (it < nodes_.end()) {
      const unsigned char* bytes = (unsigned char*)it->word.begin();
      buckets[256 * bytes[0] + bytes[1]].push_back(it->word);
      ++it;
    }
    nodes_.resize(mid - nodes_.begin());

    std::sort(nodes_.begin(), mid);
    for (int i = 0; i < 256 * 256; i++) {
      std::sort(buckets[i].begin(), buckets[i].end(),
                [](StringView a, StringView b) {
                  uint32_t len = std::min(a.length(), b.length());
                  for (uint32_t i = 2; i <= len; i++) {
                    if (a[i] != b[i]) {
                      return (unsigned char)a[i] < (unsigned char)b[i];
                    }
                  }
                  return false;
                });
      for (StringView view : buckets[i]) {
        nodes_.push_back({view, 1});
      }
    }
    auto snodes = shorts.SortedResult();
    std::vector<Node> all_nodes;
    all_nodes.reserve(nodes_.size() + snodes.size());
    size_t in = 0;
    for (auto& snode : snodes) {
      StringView view((const char*)snode.c_str, snode.length, 0);
      Node node{arena_.Allocate(view), snode.count};
      while (in < nodes_.size() && nodes_[in] < node) {
        all_nodes.push_back(nodes_[in++]);
      }
      all_nodes.push_back(node);
    }
    while (in < nodes_.size()) {
      all_nodes.push_back(nodes_[in++]);
    }
    return WordStat(std::move(arena_), std::move(all_nodes));
  }

  void MaybeRehash() {
    if (occupied_ < capacity_ * max_load_factor_) {
      return;
    }
    capacity_ *= 4;
    std::vector<Node> old_nodes(capacity_);
    nodes_.swap(old_nodes);
    for (Node& old_node : old_nodes) {
      if (old_node.count > 0) {
        *FindNode(old_node.word) = old_node;
      }
    }
  }

private:
  Node* FindNode(StringView word) {
    size_t node_id = word.hash() & (capacity_ - 1);
    for (;;) {
      if (nodes_[node_id].word == word || nodes_[node_id].count == 0) {
        return &nodes_[node_id];
      }
      node_id = (node_id + 1) & (capacity_ - 1);
    }
  }

  Arena arena_;

  std::vector<Node> nodes_;
  size_t capacity_;
  size_t occupied_;
  double max_load_factor_;
};

bool IsSpace(char ch) {
  return ch == '\n' || ch == ' ' || ch == '\t';
}

}  // namespace

WordStat wordcount(const std::string& filePath) {
  LongWordCounter long_map(0.6);
  ShortWordCounter short_map(0.6);

  FileMapping mapping(filePath);

  const char* ptr = mapping.data();
  const char* end = ptr + mapping.size();
  const char* head = ptr;
  bool space = true;
  uint32_t hash = 2166136261u;
  constexpr size_t kBatchSize = 256 * 1024;
  std::vector<StringView> words;
  words.reserve(kBatchSize);
  for (; ptr < end; ptr++) {
    if (IsSpace(*ptr)) {
      if (space) {
        continue;
      }
      space = true;
      ptrdiff_t len = ptr - head;
      words.push_back(StringView(head, len, hash));
      if (words.size() >= kBatchSize) {
        for (StringView view : words) {
          if (view.length() <= ShortWordCounter::kShortStringSize) {
            short_map.Increment(view.begin(), view.length(), view.hash());
          } else {
            long_map.Increment(view);
          }
        }
        words.clear();
        short_map.MaybeRehash();
        long_map.MaybeRehash();
      }
    } else {
      if (space) {
        head = ptr;
        hash = 2166136261u;
      }
      hash ^= (uint8_t)*ptr;
      hash *= 16777619;
      space = false;
    }
  }

  if (!space) {
    ptrdiff_t len = ptr - head;
    words.push_back(StringView(head, len, hash));
  }

  for (StringView view : words) {
    if (view.length() <= ShortWordCounter::kShortStringSize) {
      short_map.Increment(view.begin(), view.length(), view.hash());
    } else {
      long_map.Increment(view);
    }
  }

  return long_map.Result(short_map);
}

#else
// clang-format off
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
