#ifndef SORT_H
#define SORT_H

#include <algorithm>
#include <vector>

#include "table.hpp"

#ifdef _WIN32
#include <cstdlib>
#define __bswap_32(x) _byteswap_ulong(x)
#endif

inline std::vector<Entry> sort_entries(StringBuffer &string_buffer,
                                       Table &table) {
  string_buffer.append((const uint8_t *)"\x00\x00\x00", 4);

  struct SortEntry {
    uint32_t offset;
    uint32_t len;
    uint64_t sort_key;
  };

  SortEntry *sort_entries =
      (SortEntry *)my_alloc(sizeof(SortEntry) * table.size);

  size_t i = 0;

  for (auto &entry : table) {
    SortEntry &sort_entry = sort_entries[i++];
    sort_entry.offset = entry.offset;
    sort_entry.len = entry.len;

    uint32_t initial_bytes = *(uint32_t *)string_buffer.pointer(entry.offset);
    const uint32_t discard = 4 - std::min((uint32_t)4, entry.len);
    initial_bytes = __bswap_32(initial_bytes);
    initial_bytes &= ~((1 << (8 * discard)) - 1);

    sort_entry.sort_key = (((uint64_t)entry.count) << 32) | (~initial_bytes);
  }

  std::sort(sort_entries, sort_entries + table.size,
            [](const SortEntry &left, const SortEntry &right) {
              return left.sort_key > right.sort_key;
            });

  std::vector<Entry> entries;
  entries.reserve(table.size);

  for (size_t i = 0; i < table.size;) {
    const SortEntry &entry = sort_entries[i];
    const uint32_t count = entry.sort_key >> 32;

    size_t j = i;

    do {
      const SortEntry &entry = sort_entries[j];
      entries.emplace_back(Entry{entry.offset, entry.len, count});
    } while (++j < table.size && sort_entries[j].sort_key == entry.sort_key);

    std::sort(entries.begin() + i, entries.begin() + j,
              [&string_buffer](const Entry &left, const Entry &right) {
                return string_buffer.string_view(left.offset, left.len) <
                       string_buffer.string_view(right.offset, right.len);
              });

    i = j;
  }

  return std::move(entries);
}

#endif