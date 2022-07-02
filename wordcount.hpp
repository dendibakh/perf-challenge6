#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

//#define SOLUTION

// use it for init any object you may need.
void init();

#ifdef SOLUTION
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include <bitset>
#include "wyhash.h"
typedef std::size_t usize;
typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
typedef __m256i u8x32;
#endif

struct WordCount {
  int count;
  std::string word;

//#ifdef SOLUTION
//  WordCount(const u8* const ptr, const usize len, const int count)
//      : count(count), word(reinterpret_cast<const char*>(ptr), len) {}
//  WordCount(int count, std::string word) : count(count), word(word) {}
//#endif

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

#ifdef SOLUTION
struct __attribute__((packed)) Entry {
  u8 bytes[8];
  u64 hash;
  inline __attribute__((always_inline)) u64 lo() const {
    return hash & 0xffffffffff;
  }
  inline __attribute__((always_inline)) u64 len() const {
    return hash >> 40;
  }
  // I suspect that the approach I used in Zig violates strict aliasing a lot more than this approach.
  inline __attribute__((always_inline)) void setCount(const u32 count) {
    memcpy(bytes, &count, 4);
  }
  inline __attribute__((always_inline)) u32 getCount() const {
    u32 ret;
    memcpy(&ret, bytes, 4);
    return ret;
  }
  inline __attribute__((always_inline)) void setPrefix(const u8* const ptr) {
    memcpy(bytes + 4, ptr, 4);
  }
  inline __attribute__((always_inline)) void loadMoreBytes(const u8* const all_the_bytes, const usize offset) {
    memcpy(bytes, all_the_bytes + lo() + offset, 8);
  }
  inline __attribute__((always_inline)) u64 getBytesBigEndian() const {
    u64 ret;
    memcpy(&ret, bytes, 8);
    return __builtin_bswap64(ret);
  }
  inline __attribute__((always_inline)) const u8* strptr(const u8* const all_the_bytes) const {
    return all_the_bytes + lo();
  }
};

struct Iterator {
  Entry* ptr;
  const u8* const all_the_bytes;
  WordCount operator*() const {
    return WordCount{(int)ptr->getCount(), std::string((const char*)ptr->strptr(all_the_bytes), ptr->len())};
  }
  Iterator& operator++() { ptr++; return *this; }
  friend bool operator== (const Iterator& a, const Iterator& b) { return a.ptr == b.ptr; };
  friend bool operator!= (const Iterator& a, const Iterator& b) { return a.ptr != b.ptr; };
};

struct WordCountArray {
  Entry* lo;
  Entry* hi;
  // This member can't be const or DoNotOptimize crashes and I don't want to try to fix it.
  const u8* all_the_bytes;
  usize size() const { return hi - lo; }
  Iterator begin() const { return Iterator{lo, all_the_bytes}; }
  Iterator end() const { return Iterator{hi, all_the_bytes}; }
  WordCount operator[] (const usize i) const {
    return WordCount{(int)lo[i].getCount(), std::string((const char*)lo[i].strptr(all_the_bytes), lo[i].len())};
  }
};
#endif

#ifdef SOLUTION
WordCountArray wordcount(std::string fileName);
#else
std::vector<WordCount> wordcount(std::string fileName);
#endif
