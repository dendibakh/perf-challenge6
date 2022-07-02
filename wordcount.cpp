#include "wordcount.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

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
#include <cstring>
#include <memory>
#include <immintrin.h>

#ifdef _MSC_VER
#define ALWAYS_INLINED __forceinline
#define NOTINLINED __declspec(noinline)
#else
#define ALWAYS_INLINED __attribute__((always_inline))
#define NOTINLINED __attribute__((noinline))
#endif
#define INLINED ALWAYS_INLINED

// ==========================================================
// https://github.com/skarupke/flat_hash_map/blob/master/flat_hash_map.hpp
// 
//          Copyright Malte Skarupke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See http://www.boost.org/LICENSE_1_0.txt)
// 
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
class FrequencyTable
{
public:
  FrequencyTable(size_t shift)
    : pot(shift + 1)
    , mask((size_t(1) << pot) - 1)
  {
    entries.resize(mask + 1 + pot);
    entries.back().distance = 1;
  }

  ALWAYS_INLINED WordCount* entry(uint64_t hash)
  {
    return entries.data() + (hash & mask);
  }

  INLINED void emplaceHash(const std::string_view& view, uint64_t hash)
  {
    auto p = entry(hash);
    uint32_t distance = 1;
    for (; p->distance >= distance; p++, distance++) {
      if ((hash == p->hash) && (view.size() == p->word.size()) /* && !std::memcmp(view.data(), p->word.data(), view.size())*/) {
        p->count++;
        return;
      }
    }

    emplace_new_key(distance, 1, hash, FastString(view), p);
  }

  INLINED void prefetchHash(uint64_t hash)
  {
    auto p = entry(hash);
    for (int i = int(pot >> 4) + 1; --i >= 0; p++) {
      __builtin_prefetch(p, 0);
    }
  }

private:
  INLINED void emplaceHash(int count, uint64_t hash, FastString&& key)
  {
    auto p = entry(hash);
    uint32_t distance = 1;
    for (; p->distance >= distance; p++, distance++) {
      if ((hash == p->hash) && (key.size() == p->word.size()) /* && !std::memcmp(key.data(), p->word.data(), key.size())*/) {
        p->count += count;
        return;
      }
    }

    emplace_new_key(distance, count, hash, std::move(key), p);
  }

  NOTINLINED void emplace_new_key(uint32_t distance, int count, uint64_t hash, FastString&& key, WordCount* p)
  {
    using std::swap;
    if (distance == static_cast<uint32_t>(pot + 1) || count * 2 >= mask)
    {
      grow();
      emplaceHash(count, hash, std::move(key));
      return;
    }
    else if (!p->distance) {
      p->emplace(distance, count, hash, std::move(key));
      count++;
      return;
    }

    int to_insert_count = count;
    uint64_t to_insert_hash = hash;
    FastString to_insert_word(std::move(key));

    swap(distance, p->distance);
    swap(to_insert_count, p->count);
    swap(to_insert_hash, p->hash);
    swap(to_insert_word, p->word);

    auto result = p;
    distance++;
    for (;;) {
      p++;

      if (!p->distance) {
        p->emplace(distance, to_insert_count, to_insert_hash, std::move(to_insert_word));
        count++;
        return;
      }
      else if (p->distance < distance) {
        swap(distance, p->distance);
        swap(to_insert_count, p->count);
        swap(to_insert_hash, p->hash);
        swap(to_insert_word, p->word);

        distance++;
      }
      else {
        distance++;
        if (distance == static_cast<uint32_t>(pot + 1)) {
          swap(to_insert_count, result->count);
          swap(to_insert_hash, result->hash);
          swap(to_insert_word, result->word);

          grow();
          emplaceHash(to_insert_count, hash, std::move(to_insert_word));
          return;
        }
      }
    }
  }

  NOTINLINED void grow()
  {
    pot++;

    mask <<= 1;
    mask++;

    count = 0;

    std::vector<WordCount> new_buckets(mask + 1 + pot);
    new_buckets.back().distance = 1;

    std::swap(entries, new_buckets);
    new_buckets.back().distance = 0;

    for (auto& it : new_buckets) {
      if (it.distance) {
        emplaceHash(it.count, it.hash, std::move(it.word));
      }
    }
  }

public:
  std::vector<WordCount> entries;

private:
  size_t pot = 0;
  size_t mask = 0;
  size_t count = 0;
};
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// 
//          Copyright Malte Skarupke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See http://www.boost.org/LICENSE_1_0.txt)
// 
// ==========================================================

static constexpr size_t kBufferCapacity = 1 /* MB */ * 1024 * 1024;

INLINED static void FlushView(std::string_view& view, uint64_t& hash, FrequencyTable& table)
{
  if (!view.empty()) [[likely]] {
    table.emplaceHash(view, hash);
    view = {};
  }
}

NOTINLINED static size_t ReadBuffer(uint8_t* __restrict buffer, std::ifstream& inFile,
  std::string_view& viewOther1, uint64_t hashOther1,
  std::string_view& viewOther2, uint64_t hashOther2, FrequencyTable& table)
{
  FlushView(viewOther1, hashOther1, table);
  FlushView(viewOther2, hashOther2, table);

  const size_t len = inFile.rdbuf()->sgetn(reinterpret_cast<char*>(buffer), kBufferCapacity);

  // end marker
  const __m256i vend = _mm256_set1_epi16(0x0020);
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(&buffer[len]), vend);

  // replace all blanks with spaces
  const __m256i vc09 = _mm256_set1_epi8(0x09);
  const __m256i vc0A = _mm256_set1_epi8(0x0A);
  const __m256i vc0D = _mm256_set1_epi8(0x0D);
  const __m256i vc20 = _mm256_set1_epi8(0x20);

  for (size_t i = 0; i < len; i += 32) {
    __m256i vx = _mm256_loadu_si256(reinterpret_cast<__m256i*>(&buffer[i]));

    __m256i v = _mm256_or_si256(_mm256_cmpeq_epi8(vx, vc09), _mm256_cmpeq_epi8(vx, vc0A));
    v = _mm256_or_si256(v, _mm256_cmpeq_epi8(vx, vc0D));

    vx = _mm256_blendv_epi8(vx, vc20, v);

    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&buffer[i]), vx);
  }

  return len;
}

INLINED static std::string_view ReadWord(uint8_t* __restrict buffer, size_t& cur, size_t& len, std::ifstream& inFile,
  uint64_t& hash,
  std::string_view& viewOther1, const uint64_t& hashOther1,
  std::string_view& viewOther2, const uint64_t& hashOther2, FrequencyTable& table)
{
  uint8_t ch;
  for (;;) {
    // eat spaces
    do {
      ch = buffer[cur++];
    } while (ch == 0x20);

    // buffer has been processed
    if (cur > len) [[unlikely]] {
      len = ReadBuffer(buffer, inFile, viewOther1, hashOther1, viewOther2, hashOther2, table);
      cur = 0;

      // eof
      if (cur == len) {
        return {};
      }

      continue;
    }

    break;
  }

  // eat word
  auto text = &buffer[cur - 1];

  uint64_t h = 0xcbf29ce484222325uLL;
  do {
    // http://isthe.com/chongo/tech/comp/fnv/#FNV-1a
    h ^= ch;
    h *= 0x100000001b3uLL;

    ch = buffer[cur++];
  } while (ch != 0x20);

  size_t count = &buffer[cur - 1] - text;

  // incomplete word
  if (cur > len) [[unlikely]] {
    inFile.rdbuf()->pubseekoff(-std::streamoff(count), std::ios_base::cur, std::ios_base::in);

    len = ReadBuffer(buffer, inFile, viewOther1, hashOther1, viewOther2, hashOther2, table);
    cur = 0;

    // eat word
    ch = buffer[cur++];

    h = 0xcbf29ce484222325uLL;
    do {
      // http://isthe.com/chongo/tech/comp/fnv/#FNV-1a
      h ^= ch;
      h *= 0x100000001b3uLL;

      ch = buffer[cur++];
    } while (ch != 0x20);

    count = cur - 1;
    text = buffer;
  }

  hash = h;
  return { reinterpret_cast<const char*>(text), count };
}

std::vector<WordCount> wordcount(std::string filePath) {
  std::ifstream inFile{ filePath, std::ios_base::in | std::ios_base::binary };
  if (!inFile) {
    std::cerr << "Invalid input file: " << filePath << "\n";
    return {};
  }

  FrequencyTable table(8);

  {
    std::unique_ptr<uint8_t[]> bufferHolder = std::make_unique<uint8_t[]>(kBufferCapacity + 64);

    const auto buffer = bufferHolder.get();
    buffer[0] = 0;

    std::string_view view1, view2, view3;
    uint64_t hash1 = 0, hash2 = 0, hash3 = 0;

    size_t cur = 0;
    size_t len = 0;

    for (;;) {
      // process first word
      if (!view1.empty()) [[likely]] {
        table.emplaceHash(view1, hash1);
      }

      view1 = ReadWord(buffer, cur, len, inFile, hash1,
        view2, hash2, view3, hash3, table);
      if (view1.empty()) [[unlikely]] {
        break;
      }

      table.prefetchHash(hash1);

      // process second word
      if (!view2.empty()) [[likely]] {
        table.emplaceHash(view2, hash2);
      }

      view2 = ReadWord(buffer, cur, len, inFile, hash2,
        view1, hash1, view3, hash3, table);
      if (view2.empty()) [[unlikely]] {
        break;
      }

      table.prefetchHash(hash2);

      // process third word
      if (!view3.empty()) [[likely]] {
        table.emplaceHash(view3, hash3);
      }

      view3 = ReadWord(buffer, cur, len, inFile, hash3,
        view1, hash1, view2, hash2, table);
      if (view3.empty()) [[unlikely]] {
        break;
      }

      table.prefetchHash(hash3);
    }

    FlushView(view1, hash1, table);
    FlushView(view2, hash2, table);
    FlushView(view3, hash3, table);
  }

  // remove empty entries
  std::vector<WordCount> mvec;
  std::swap(mvec, table.entries);
  {
    auto e = --mvec.end();
    e->distance = 0;

    // skip used
    auto w = mvec.begin();
    while (w->distance) {
      ++w;
    }

    // compact
    if (w != e) {
      for (auto it = w + 1; it != e; ++it) {
        const bool add = !!it->distance;
        new(&*w) WordCount(std::move(*it));
        w += add;
      }
    }

    mvec.resize(std::distance(mvec.begin(), w));
  }

  // letters reverse
  struct Prefix {
    __m128i raw;
  } prefix;

  const __m128i mlen[1 + sizeof(uint64_t) + sizeof(uint32_t)] = {
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9),
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9),
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 9),

    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9),
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9, 9),
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9),
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9, 9),

    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9, 9, 9),
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9),
    _mm_set_epi8(0, 0, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9),
    _mm_set_epi8(0, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9),

    _mm_set_epi8(0, 0, 0, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9)
  };

  const __m128i m9 = _mm_set1_epi8(9);

  // cache beginnings of each word
  for (auto& item : mvec) {
    const auto p = item.word.data();
    const auto n = item.word.size();

    prefix.raw = _mm_setzero_si128();

    const size_t len = std::min(n, sizeof(uint64_t) + sizeof(uint32_t));
    std::memcpy(&prefix, p, len);

    __m128i mv = prefix.raw;
    // remap chars [0..8] to [1..9] where input char 9 is always absent
    __m128i mlt = mlen[len];
    mv = _mm_sub_epi8(mv, _mm_cmpeq_epi8(_mm_max_epu8(mlt, mv), m9));
    // apply big endian
    mv = _mm_shuffle_epi8(mv, _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7));

    item.hash = static_cast<uint64_t>(_mm_cvtsi128_si64(mv));
    item.distance = static_cast<uint32_t>(_mm_cvtsi128_si64(_mm_unpackhi_epi64(mv, mv)));
  }

  // final sort
  std::sort(mvec.begin(), mvec.end(),
    [](const WordCount& lhs, const WordCount& rhs) -> bool
    {
      if (lhs.count > rhs.count) {
        return true;
      }

      if (lhs.count == rhs.count) {
        if (lhs.hash < rhs.hash) {
          return true;
        }

        if (lhs.hash == rhs.hash) {
          if (lhs.distance < rhs.distance) {
            return true;
          }

          if (lhs.distance == rhs.distance) {
            return lhs.word < rhs.word;
          }
        }
      }

      return false;
    });

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
