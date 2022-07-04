#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "table.hpp"

struct StringBuffer {
  size_t len;
  size_t capacity;
  uint8_t *buffer;

  StringBuffer(size_t size_hint)
      : len(0), capacity(std::max((size_t)2048, size_hint)),
        buffer((uint8_t *)my_alloc(capacity)) {}

  StringBuffer(StringBuffer &&other)
      : buffer(other.buffer), len(other.len), capacity(other.capacity) {
    other.buffer = NULL;
    other.len = 0;
    other.capacity = 0;
  }

  uint32_t append(const uint8_t *data, size_t len) {
    const uint32_t offset = this->len;

    if (this->len + len > capacity) {
      uint8_t *old_buffer = buffer;
      size_t old_capacity = capacity;

      capacity = 2 * capacity + len;
      buffer = (uint8_t *)my_alloc(capacity);
      memcpy(buffer, old_buffer, old_capacity);

      my_free(old_buffer, old_capacity);
    }

    assert(this->len + len <= capacity);

    memcpy(this->buffer + this->len, data, len);
    this->len += len;

    return offset;
  }

  uint8_t *pointer(uint32_t offset) { return buffer + offset; }

  std::string_view string_view(uint32_t offset, uint32_t len) {
    return std::string_view((const char *)buffer + offset, len);
  }
};

#endif