#ifndef INPUT_H
#define INPUT_H

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "table.hpp"

struct Input {
  static const size_t buffer_size = 8 * 1024 * 1024;
  static const size_t buffer_alignment = 32;
  static const size_t buffer_padding = 32;

  FILE *file;
  uint8_t *buffer;

  Input(std::string filename)
      : buffer((uint8_t *)my_alloc(buffer_size + buffer_padding)) {
    const char *filename_c_str = filename.c_str();

    if ((file = fopen(filename_c_str, "r")) == NULL) {
      abort();
    }
  }

  std::pair<uint8_t *, size_t> read_next() {
    const long len = fread(buffer, 1, buffer_size, file);

    if (len < 0) {
      abort();
    }

    return std::make_pair(buffer, (size_t)len);
  }
};

#endif