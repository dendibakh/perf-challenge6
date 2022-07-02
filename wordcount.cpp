#include "wordcount.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <chrono>
#include <type_traits>

#include <cstring>
#include <cstdint>
#include <cassert>

#include <x86intrin.h>

#include "platform_specific.hpp"
#include "aligned_allocator.hpp"
#include "pdqsort.h"

#define BUCKET_COUNT (4194304*2*2*4) // 2^22

#define __assume(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)

#ifdef SOLUTION

uint32_t crcHash(const uint8_t* key, int64_t len)
{
    uint64_t crc = 0;
    while (len > 8)
    {
        uint64_t val = *(const uint64_t*)key;
        crc = _mm_crc32_u64(crc, val);
        len -= 8;
        key += 8;
    }

    // CRC the final 1-7 bytes
    uint64_t val = *(const uint64_t*)key;
    val &= ~(~0ULL << len*8); // Compiles to a bzhi instruction (also UB)
    crc = _mm_crc32_u64(crc, val);

    return crc;
}

inline uint32_t crcHash32B(uint64_t q0, uint64_t q1, uint64_t q2, uint64_t q3)
{
    uint64_t crc = 0;
    crc = _mm_crc32_u64(crc, q0);
    crc = _mm_crc32_u64(crc, q1);
    crc = _mm_crc32_u64(crc, q2);
    crc = _mm_crc32_u64(crc, q3);

    return crc;
}

struct __attribute__((packed)) hash_slot_t
{
    uint32_t count;
    __m256i node;
};

struct hash_table_t
{
    hash_slot_t slots[BUCKET_COUNT];
    int unique_words;
};

// Generate a mask for clearing the n upper bytes of a vector
inline __m256i mask_shift_2(uint32_t n)
{
    static const int8_t mask_lut[64] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    return _mm256_loadu_si256((__m256i *)(mask_lut + 32 - n));
}

inline void hash_table_push(hash_table_t* table, uint32_t hash, __m256i val, int len, const uint8_t* ptr)
{
    hash %= BUCKET_COUNT;

    hash_slot_t* slot = &table->slots[hash];

    if (_mm256_movemask_epi8(_mm256_cmpeq_epi8(slot->node, val)) == 0xFFFFFFFF)
    {
        ++slot->count;
        return;
    }

    bool relooped = false;

    loop:
    for (; slot < &table->slots[BUCKET_COUNT]; ++slot)
    {
        if (slot->count == 0)
        {
            slot->count = 1;
            slot->node = val;
            return;
        }

        if (_mm256_movemask_epi8(_mm256_cmpeq_epi8(slot->node, val)) == 0xFFFFFFFF)
        {
            ++slot->count;
            return;
        }
    }

    if (!relooped)
    {
        relooped = true;
        slot = &table->slots[0];
        goto loop;
    }
    else
    {
        fprintf(stderr, "COLLISION OVERFLOW");
        exit(1);
        return;
    }
}

char* string_buffer;
char* string_buffer_ptr;

inline void hash_table_push_ptr(hash_table_t* table, uint32_t hash, int len, const uint8_t* ptr)
{
    hash %= BUCKET_COUNT;

    hash_slot_t* slot = &table->slots[hash];
    bool relooped = false;

    uint32_t len_in_qwords = (len / 8) + (len % 8 ? 1 : 0);

    loop:
    do
    {
        // nonzero, it's not a pointer of the same length, skip
        if (__builtin_expect(slot->node[3] != len, 0))
        {
            if (__builtin_expect(slot->node[0] == 0, 1))
            {
                slot->count = 1;
                slot->node = _mm256_set_epi64x(len, ((uint64_t*)ptr)[0], (uint64_t)string_buffer_ptr, 0u | ((uint64_t)len_in_qwords << 32u));

                memcpy(string_buffer_ptr, ptr, len);
                string_buffer_ptr += len;
                // Pad
                string_buffer_ptr += (8 - len%8) + 8;

                return;
            }
            else
                continue;
        }
		if (slot->node[2] != ((uint64_t*)ptr)[0]) // First 8 bytes differ
			continue;
		
        uint8_t* other_ptr = *((uint8_t**)&slot->node + 1);
        if (__builtin_expect(memcmp(ptr+16, other_ptr+16, len-16) == 0, 1))
        {
            ++slot->count;

            return;
        }
    } while (++slot < &table->slots[BUCKET_COUNT]);

    if (!relooped)
    {
        relooped = true;
        slot = &table->slots[0];
        goto loop;
    }
    else
    {
        fprintf(stderr, "COLLISION OVERFLOW");
        exit(1);
        return;
    }
}

inline const uint8_t* skip_whitespace(const uint8_t* ptr)
{
    do
        ++ptr;
    while ((uint8_t)(*ptr-1) < ' ');
    return ptr;
}

static hash_table_t* hash_table;
void read_words(const std::string& filePath)
{
    void* large_ptr = (hash_table_t*)alloc_large_pages(sizeof(hash_table_t));
    if (!large_ptr)
    {
        fprintf(stderr, "Unable to allocate the hash_table pointer\n");
        exit(1);
    }
	
	string_buffer_ptr = string_buffer = (char*)alloc_large_pages(2000000000);
    if (!string_buffer)
    {
        fprintf(stderr, "Unable to allocate the string_buf pointer\n");
        exit(1);
    }

    hash_table = (hash_table_t*)large_ptr;

    const uint8_t* buf = (const uint8_t*)getDataPtr(filePath);
    const uint8_t* buf_base = buf;
    while (*buf == ' ' || *buf == '\t' || *buf == '\n'|| *buf == '\r')
        ++buf;

    const uint8_t* boundary = buf;
    do
    {
        // Try finding the first non-graphical char
        __m256i chunk = _mm256_loadu_si256((const __m256i*)buf);
        // unsigned comparison
        __m256i boundaries = _mm256_cmpeq_epi8(_mm256_set1_epi8(' '), _mm256_max_epu8(chunk,_mm256_set1_epi8(' ')));


        uint32_t mask = _mm256_movemask_epi8(boundaries);

        if (mask == 0) // >32B word
        {
            boundary += sizeof(__m256i);
            while (*boundary > ' ') // Skip until reaching a non-word character
                ++boundary;

            uint32_t hash = crcHash(buf, (uint32_t)(boundary - buf));

            hash_table_push_ptr(hash_table, hash, (uint32_t)(boundary - buf), buf);
        }
        else
        {
            int len = _tzcnt_u32(mask);

            chunk = _mm256_and_si256(chunk, mask_shift_2(len));

            boundary += len;


            uint32_t hash = crcHash32B(_mm256_extract_epi64(chunk, 0),
                                       _mm256_extract_epi64(chunk, 1),
                                       _mm256_extract_epi64(chunk, 2),
                                       _mm256_extract_epi64(chunk, 3));

            hash_table_push(hash_table, hash, chunk, len, buf);
        }

        boundary = skip_whitespace(boundary);
        buf = boundary;
    } while (*buf);
}

struct frequent_item_t
{
	__m256i val;
	uint32_t count;
};

template <typename T>
void do_sort(T* begin, T* end)
{
    pdqsort(begin, end, [](const T& lhs, const T& rhs)
    {
        if constexpr (std::is_same_v<T, frequent_item_t>)
        {
            if (lhs.count != rhs.count)
                return lhs.count > rhs.count;
        }

        uint64_t* lhs_ptr = (uint64_t*)&lhs;
        uint64_t* rhs_ptr = (uint64_t*)&rhs;

        if (*(uint8_t*)lhs_ptr == 0 || *(uint8_t*)rhs_ptr == 0) // Pointer
        {
            bool invert = false;
            if (*(uint8_t*)rhs_ptr == 0)
            {
                invert = true;
                std::swap(lhs_ptr, rhs_ptr);
            }

            int lhs_len = lhs_ptr[0] >> 32;
            int rhs_len;
            uint64_t* lhs = (uint64_t*)lhs_ptr[1];
            uint64_t* rhs;

            if (*(uint8_t*)rhs_ptr == 0) // Pointer vs pointer
            {
                rhs_len = rhs_ptr[0] >> 32;
                rhs = (uint64_t*)rhs_ptr[1];
            }
            else // Pointer vs inline
            {
                rhs_len = 32/sizeof(uint64_t);
                rhs = rhs_ptr;
            }

			// First qword is stored in-place
			for (int i = 0; i < 1; ++i)
			{
				if (__builtin_bswap64(lhs_ptr[i+2]) != __builtin_bswap64(*rhs))
					return invert ? __builtin_bswap64(lhs_ptr[i+2]) > __builtin_bswap64(*rhs) :
									__builtin_bswap64(lhs_ptr[i+2]) < __builtin_bswap64(*rhs);
				++lhs; ++rhs; --rhs_len;
			}

            do
            {
                if (__builtin_bswap64(*lhs) != __builtin_bswap64(*rhs))
                    return invert ? __builtin_bswap64(*lhs) > __builtin_bswap64(*rhs) :
                                    __builtin_bswap64(*lhs) < __builtin_bswap64(*rhs);
                ++lhs; ++rhs;
            } while (rhs_len-- > 0);
            return false;
        }

        for (int i = 0; i < 32/sizeof(uint64_t); ++i)
        {
            if (__builtin_bswap64(*lhs_ptr) != __builtin_bswap64(*rhs_ptr))
                return __builtin_bswap64(*lhs_ptr) <     __builtin_bswap64(*rhs_ptr);
            ++lhs_ptr; ++rhs_ptr;
        }
        return false;
    });
}

#define MEDIUM_SORT_THRESHOLD 32

std::vector<WordCount> wordcount(std::string filePath) {
	
	enable_large_pages();
	
	std::vector<__m256i, aligned_allocator<__m256i, sizeof(__m256i)>> medium_sort_arrays[MEDIUM_SORT_THRESHOLD];
	std::vector<frequent_item_t, aligned_allocator<frequent_item_t, sizeof(__m256i)>> frequent_items;
	
    std::vector<__m256i, aligned_allocator<__m256i, sizeof(__m256i)>> sort_buffers[256];
	for (auto& vec : sort_buffers)
		vec.reserve(80000);
	
	read_words(filePath);

	int total_entries = 0;
    for (size_t i = 0; i < BUCKET_COUNT; ++i)
    {
        if (hash_table->slots[i].count == 0)
            continue;

        if (hash_table->slots[i].count == 1)
        {
			__m256i node = _mm256_loadu_si256(&hash_table->slots[i].node);
			
            uint8_t c;
            if (*(uint8_t*)&hash_table->slots[i].node == 0)
                c = ((uint8_t*)&node)[2*sizeof(uint64_t)]; // first 16bytes are stored in place, grab the first char from it
            else
                c = *(uint8_t*)&hash_table->slots[i].node;

            sort_buffers[c].push_back(node);
			++total_entries;
        }
        else if (hash_table->slots[i].count <= MEDIUM_SORT_THRESHOLD)
        {
			__m256i node = _mm256_loadu_si256(&hash_table->slots[i].node);
            medium_sort_arrays[hash_table->slots[i].count-1].push_back(node);
			++total_entries;
        }
        else
        {
			__m256i node = _mm256_loadu_si256(&hash_table->slots[i].node);
            frequent_items.push_back((frequent_item_t){node, hash_table->slots[i].count});
			++total_entries;
        }
    }

    for (int c = 0; c < 256; ++c)
        do_sort(sort_buffers[c].data(), sort_buffers[c].data() + sort_buffers[c].size());

    for (int i = 0; i < MEDIUM_SORT_THRESHOLD; ++i)
    {
        do_sort(medium_sort_arrays[i].data(), medium_sort_arrays[i].data() + medium_sort_arrays[i].size());
    }
    do_sort(frequent_items.data(), frequent_items.data() + frequent_items.size());

	std::vector<WordCount> mvec;
	mvec.reserve(total_entries);
	
	auto add_string = [&mvec](const __m256i& node, uint32_t count)
	{
		if (*(uint8_t*)&node == 0) // Pointer
		{
			const char* ptr = (const char*)node[1];
			size_t len = node[3];
			mvec.emplace_back(WordCount{(int)count, std::string(ptr, len)});
		}
		else // Pointer vs inline
		{
			const char* ptr = (const char*)&node;
			__m256i mask = _mm256_cmpeq_epi8(node, _mm256_set1_epi8(0));
			int len = _mm_tzcnt_32(_mm256_movemask_epi8(mask));
			mvec.emplace_back(WordCount{(int)count, std::string(ptr, len)});
		}
	};
	
	for (const auto& item : frequent_items)
		add_string(item.val, item.count);
    for (int i = MEDIUM_SORT_THRESHOLD-1; i >= 0; --i)
    {
		for (const auto& node : medium_sort_arrays[i])
			add_string(node, i+1);
    }
    for (int i = 0; i < 256; ++i)
    {
		for (const auto& node : sort_buffers[i])
			add_string(node, 1);
    }
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