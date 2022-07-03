#ifndef WORD_COUNT_H
#define WORD_COUNT_H

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <immintrin.h>

#include "input.hpp"
#include "sort.hpp"
#include "string_buffer.hpp"
#include "table.hpp"

struct WordCountEntry {
  std::string_view word;
  uint32_t count;
};

struct WordCountResult {
  StringBuffer string_buffer;
  std::vector<Entry> entries;

  struct ResultIter {
    WordCountResult *result;
    size_t i;

    WordCountEntry operator*() { return (*result)[i]; }

    ResultIter &operator++() {
      i++;
      return *this;
    }

    bool operator!=(ResultIter &other) { return i != other.i; }
  };

  ResultIter begin() { return ResultIter{this, 0}; }

  ResultIter end() { return ResultIter{this, entries.size()}; }

  size_t size() { return entries.size(); }

  WordCountEntry operator[](size_t i) {
    Entry *entry = &entries[i];
    return WordCountEntry{string_buffer.string_view(entry->offset, entry->len),
                          entry->count};
  }
};

inline WordCountResult wordcount(std::string filename) {
  Input input(filename);

  Table table(512 * 1024, 1024 * 1024);

  std::vector<uint8_t> spill;
  spill.reserve(1024);

  for (;;) {
    std::pair<uint8_t *, size_t> pair = input.read_next();
    uint8_t *buffer = pair.first;
    size_t len = pair.second;

    if (len == 0) {
      if (spill.size() != 0) {
        table.increment(spill.data(), spill.size());
      }
      break;
    }

    _mm256_storeu_si256((__m256i *)(buffer + len), _mm256_setzero_si256());

    size_t start = 0;

    for (size_t i = 0; i < len; i += 32) {
      const __m256i block = _mm256_load_si256((const __m256i *)(buffer + i));

      uint32_t whitespace_mask = _mm256_movemask_epi8(_mm256_or_si256(
          _mm256_cmpeq_epi8(block, _mm256_set1_epi8(' ')),
          _mm256_or_si256(_mm256_cmpeq_epi8(block, _mm256_set1_epi8('\t')),
                          _mm256_cmpeq_epi8(block, _mm256_set1_epi8('\n')))));

      if (whitespace_mask == 0) {
        continue;
      }

      if (spill.size() != 0) {
        const size_t offset = __builtin_ctz(whitespace_mask);
        whitespace_mask ^= 1 << offset;

        const size_t end = i + offset;

        spill.insert(spill.end(), buffer + start, buffer + end);
        table.increment(spill.data(), spill.size());

        spill.clear();

        start = end + 1;
      }

      while (whitespace_mask != 0) {
        const size_t offset = __builtin_ctz(whitespace_mask);
        whitespace_mask ^= 1 << offset;

        const size_t end = i + offset;

        if (start != end) {
          table.increment(buffer + start, end - start);
        }

        start = end + 1;
      }
    }

    spill.insert(spill.end(), buffer + start, buffer + len);
  }

  StringBuffer string_buffer = std::move(table.string_buffer);
  std::vector<Entry> entries = sort_entries(string_buffer, table);

  return WordCountResult{std::move(string_buffer), std::move(entries)};
}

#endif