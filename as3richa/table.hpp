#ifndef TABLE_H
#define TABLE_H

#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <string>

#include "alloc.hpp"
#include "string_buffer.hpp"

inline size_t hash(std::string_view data) {
  return std::hash<std::string_view>{}(data);
}

inline size_t ceiling_pow2(size_t value) {
  const size_t leading_zeros = __builtin_clzll(value);

  const size_t ceiling_trailing_zeros =
      sizeof(size_t) * __CHAR_BIT__ - leading_zeros;

  return 1 << ceiling_trailing_zeros;
}

struct Entry {
  uint32_t offset;
  uint32_t len;
  uint32_t count;
};

struct Table {
  const double max_load_factor = 0.5;

  size_t size;
  size_t capacity;
  Entry *buffer;

  StringBuffer string_buffer;

  struct TableIter {
    Table *table;
    size_t i;

    TableIter(Table *table, size_t i) : table(table), i(i) { next(); }

    Entry &operator*() { return table->buffer[i]; }

    TableIter &operator++() {
      i++;
      next();
      return *this;
    }

    bool operator!=(TableIter &other) { return i != other.i; }

    void next() {
      for (; i < table->capacity && table->buffer[i].len == 0; i++)
        ;
    }
  };

  Table(size_t size_hint, size_t string_buffer_size_hint)
      : size(0), capacity(std::max((size_t)2048, ceiling_pow2(size_hint))),
        buffer((Entry *)my_alloc(sizeof(Entry) * capacity)),
        string_buffer(string_buffer_size_hint) {}

  ~Table() { my_free(buffer, sizeof(Entry) * capacity); }

  void increment(const uint8_t *data, uint32_t len) {
    assert(len != 0);

    const size_t hash_value = hash(std::string_view((const char *)data, len));

    for (size_t i = hash_value & (capacity - 1);;
         i = (i + 1) & (capacity - 1)) {
      Entry *entry = &buffer[i];

      if (entry->len == 0) {
        *entry = Entry{string_buffer.append(data, len), len, 1};

        size++;
        if ((double)size / capacity >= max_load_factor) {
          rehash();
        }

        break;
      }

      if (string_buffer.string_view(entry->offset, entry->len) ==
          std::string_view((const char *)data, len)) {
        entry->count++;
        break;
      }
    }
  }

  TableIter begin() { return TableIter(this, 0); }

  TableIter end() { return TableIter(this, capacity); }

  void rehash() {
    const size_t old_capacity = capacity;
    const Entry *old_buffer = buffer;

    capacity *= 2;
    buffer = (Entry *)my_alloc(sizeof(Entry) * capacity);

    for (size_t i = 0; i < old_capacity; i++) {
      const Entry *old_entry = &old_buffer[i];

      if (old_entry->len == 0) {
        continue;
      }

      const size_t hash_value =
          hash(string_buffer.string_view(old_entry->offset, old_entry->len));

      for (size_t j = hash_value & (capacity - 1);;
           j = (j + 1) & (capacity - 1)) {
        Entry *entry = &buffer[j];

        if (entry->len == 0) {
          *entry = *old_entry;
          break;
        }
      }
    }

    my_free((void*)old_buffer, sizeof(Entry) * capacity);
  }
};

#endif