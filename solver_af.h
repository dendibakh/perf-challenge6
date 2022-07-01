#pragma once
#include "wordcount.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <string_view>
#include "StringView.h"
#include "robin_map.h"

struct Buffer
{
    size_t capacity;
    size_t size;
    char* data;

    const char* end() const {
        return data + size;
    }
};

template<typename T, typename U>
T convert(const U string);

template<>
std::string convert<std::string, StringView>(StringView v) {
    return v.str();
}

template<>
std::string convert<std::string, std::string_view>(std::string_view v) {
    return std::string(v);
}

template<>
std::string convert<std::string, std::string>(std::string v) {
    return v;
}

template<>
StringView convert<StringView, StringView>(StringView v) {
    return v;
}

constexpr size_t max_word_size = 1000;

constexpr bool IsWhitespace(char c)
{
    return c == 9 || c == 10 || c == 13 || c == 32;
}

Buffer read_chunk(FILE* file)
{
    constexpr size_t max_chunk_size = 1000000;
    constexpr size_t to_read = max_chunk_size - max_word_size;

    Buffer b;
    b.data = new char[max_chunk_size];
    b.capacity = max_chunk_size;
    b.size = 0;

    int readed = fread(b.data, 1, to_read, file);
    if (!readed)
        return b;

    char* last = b.data + readed - 1;
    while (!IsWhitespace(*last) && readed) {
        ++last;
        readed = fread(last, 1, 1, file);
        if(!readed) *last = ' ';
    }

    ++last;
    *last = ' ';
    b.size = last - b.data;

    return b;
}

using map_string_t = StringView;
using sort_string_t = StringView;

std::vector<Buffer> chunks;
tsl::robin_map<map_string_t, int> m;

void read_input(FILE* file) {
    while (true) {
        Buffer b = read_chunk(file);
        if (b.size <= 1)
            break;
        chunks.push_back(b);

        const char* p = b.data;
        const char* end = b.end();

        while (p < end)
        {
            //skip whitespaces
            while (IsWhitespace(*p) && p < end) {
                ++p;
            }
            if (p == end) break;

            const char* word_begin = p;
            ++p;

            while (!IsWhitespace(*p) && p < end) {
                ++p;
            }

            map_string_t word( word_begin, (int)(p - word_begin) );
            //std::string_view word{ word_begin, size_t(p - word_begin) };
            --m[word];
        }

    }
}

std::vector<std::pair<int, sort_string_t>> mvec;

void sort_items()
{
    using item_t = std::pair<int, sort_string_t>;
    sort(mvec.begin(), mvec.end(), [](const item_t& a, const item_t& b) {return a.first < b.first; });

    int start_i = 0;
    int end_i = 0;
    while (end_i < mvec.size())
    {
        start_i = end_i ;
        end_i = start_i + 1;
        int ref = mvec[start_i].first;
        while (end_i < mvec.size() && mvec[end_i].first == ref) ++end_i;
        std::sort(mvec.begin() + start_i, mvec.begin() + end_i, [](const item_t& a, const item_t& b) {
            return a.second < b.second; 
        });
    }
}

std::vector<WordCount> wordcount_af(std::string filePath) {
  FILE* file = fopen(filePath.c_str(), "rb");
  read_input(file);
  fclose(file);

  mvec.clear();
  mvec.reserve(m.size());
  for (auto& p : m) {
      mvec.emplace_back(p.second, convert<sort_string_t>(p.first));
  }

  sort_items();

  std::vector<WordCount> result;
  result.reserve(mvec.size());
  for(int i = 0; i < mvec.size(); ++i)
    result.emplace_back(WordCount{-mvec[i].first, mvec[i].second.str()});

  return result;
}