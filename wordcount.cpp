#include "wordcount.hpp"

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

#include "robin_hood.h"

#include <immintrin.h>
#include <limits.h>

#include <algorithm>
#include <optional>

auto getWhitespaceMask(__m256i vector) -> unsigned
{
    const auto space_mask = _mm256_set1_epi8(' ');
    const auto tab_mask   = _mm256_set1_epi8('\t');
    const auto endl_mask  = _mm256_set1_epi8('\n');

    const auto space_cmp    = _mm256_cmpeq_epi8(vector, space_mask);
    const auto tab_cmp      = _mm256_cmpeq_epi8(vector, tab_mask);
    const auto endl_cmp     = _mm256_cmpeq_epi8(vector, endl_mask);
    const auto reduct1      = _mm256_or_si256(space_cmp, tab_cmp);
    const auto reduct_final = _mm256_or_si256(reduct1, endl_cmp);
    return bit_cast< unsigned >(_mm256_movemask_epi8(reduct_final));
}

constexpr auto isWhitespace(char c) -> bool
{
    return c == ' ' or c == '\t' or c == '\n';
}

struct Hash
{
    auto operator()(const BufferedStringView& sv) const -> std::size_t
    {
        if (sv.len <= BufferedStringView::buf_size)
            return robin_hood::hash_bytes(sv.buf.data(), sv.len);
        else
            return robin_hood::hash_bytes(sv.data, sv.len);
    }
};
struct Cmp
{
    auto operator()(const BufferedStringView& sv1, const BufferedStringView& sv2) const -> bool
    {
        if (sv1.len != sv2.len)
            return false;
        if (not bufCompareEqual(sv1.buf, sv2.buf))
            return false;
        if (sv1.len <= BufferedStringView::buf_size)
            return true;
        return sv1.viewPastBuf() == sv2.viewPastBuf();
    }
};
using dict_t = robin_hood::unordered_flat_map< BufferedStringView, int, Hash, Cmp, 64 >;

auto makeDictionary(std::string_view text) -> dict_t
{
    constexpr auto smaller = [](unsigned a, std::size_t b) {
        // Windows headers define `min` as a macro, so we can't use std::min. Absolutely wild...
        return a < b ? a : b;
    };

    constexpr std::size_t small_size = 3;
    struct SmallMapEntry
    {
        int         count;
        unsigned    size;
        const char* data;
    };
    std::vector< SmallMapEntry > small_map(1ul << small_size * CHAR_BIT, SmallMapEntry{0, 0u, nullptr});

    dict_t      word_counts(20'000'000);
    std::size_t word_begin = 0, i = 0;
    const auto  add_word = [&](std::size_t word_len) {
        const auto word_ptr = text.data() + word_begin;
        if (word_len <= small_size)
        {
            unsigned str{};
            std::memcpy(&str, word_ptr, word_len); // ENDIANNESS!!!
            ++small_map[str].count;
            small_map[str].size = word_len;
            small_map[str].data = word_ptr;
        }
        else
        {
            BufferedStringView word{};
            word.len  = word_len;
            word.data = word_ptr;
            std::copy(word_ptr, word_ptr + smaller(word_len, word.buf.size()), word.buf.begin());
            ++word_counts[word];
        }
    };
    constexpr std::size_t simd_width = 32;
    for (; i + simd_width <= text.size(); i += simd_width)
    {
        const auto vector  = robin_hood::detail::unaligned_load< __m256i >(text.data() + i);
        auto       ws_mask = getWhitespaceMask(vector);
        while (ws_mask)
        {
            const auto ws_local_pos  = safeCtz(ws_mask);
            const auto ws_global_pos = i + ws_local_pos;
            const auto word_len      = ws_global_pos - word_begin;
            if (word_len > 0)
                add_word(word_len);
            word_begin = ws_global_pos + 1;
            ws_mask &= ~(1 << ws_local_pos);
        }
    }
    for (; i < text.size(); ++i)
    {
        if (isWhitespace(text[i]))
        {
            const auto word_len = i - word_begin;
            if (word_len > 0)
                add_word(word_len);
            word_begin = i + 1;
        }
    }
    const auto word_len = text.size() - word_begin;
    if (word_len > 0)
        add_word(word_len);

    for (const auto& [count, size, data] : small_map)
        if (count > 0)
        {
            BufferedStringView word{};
            word.len = size;
            std::copy(data, data + size, word.buf.begin());
            word.data         = data;
            word_counts[word] = count;
        }

    return word_counts;
}

class BucketManager
{
public:
    explicit BucketManager(const dict_t& dict)
    {
        robin_hood::unordered_map< int, unsigned > bucket_caps(1u << 11);
        for (const auto& [word, count] : dict)
            ++bucket_caps[count];
        m_counts.reserve(bucket_caps.size());
        for (const auto& [count, cap] : bucket_caps)
            m_counts.push_back(count);
        std::sort(m_counts.begin(), m_counts.end(), std::greater<>{});
        unsigned suffix_scan_tally = 0;
        for (auto c : m_counts)
        {
            auto& [offset, size] = m_bucket_info[c];
            offset               = suffix_scan_tally;
            suffix_scan_tally += bucket_caps[c];
        }
    }

    auto makeBuckets(const dict_t& map) -> std::vector< BufferedStringView >
    {
        auto bucket_vec = std::vector< BufferedStringView >(map.size());
        for (const auto& [word, count] : map)
        {
            auto& [offset, size]        = m_bucket_info[count];
            bucket_vec[offset + size++] = word;
        }
        return bucket_vec;
    }

    void sortBuckets(std::vector< BufferedStringView >& bucket_vec) const
    {
        for (auto count : m_counts)
        {
            const auto& [offset, size] = m_bucket_info.find(count)->second;
            const auto bucket_begin    = std::next(bucket_vec.begin(), offset);
            const auto bucket_end      = std::next(bucket_begin, size);

            // Pre-sort using just the buffer
            std::sort(bucket_begin, bucket_end, [](const BufferedStringView& s1, const BufferedStringView& s2) {
                return bufCompareLess(s1.buf, s2.buf);
            });

            // Sort subranges of equal SSVO buffers (hopefully there are only a few)
            auto subrange_begin = bucket_begin;
            while (subrange_begin != bucket_end)
            {
                subrange_begin = std::adjacent_find(
                    subrange_begin, bucket_end, [](const BufferedStringView& s1, const BufferedStringView& s2) {
                        return bufCompareEqual(s1.buf, s2.buf);
                    });
                const auto subrange_end =
                    std::find_if_not(subrange_begin, bucket_end, [&subrange_begin](const BufferedStringView& sv) {
                        return bufCompareEqual(subrange_begin->buf, sv.buf);
                    });
                std::sort(subrange_begin, subrange_end, [](const BufferedStringView& s1, const BufferedStringView& s2) {
                    return s1.viewPastBuf() < s2.viewPastBuf();
                });
                subrange_begin = subrange_end;
            }
        }
    }

    [[nodiscard]] auto makeResult(const std::vector< BufferedStringView >& bucket_array, MappedFile&& file) const
        -> WordCountVec
    {
        WordCountVec result{std::move(file)};
        result->reserve(bucket_array.size());
        for (auto count : m_counts)
        {
            const auto& [offset, size] = m_bucket_info.find(count)->second;
            const auto bucket_begin    = std::next(bucket_array.begin(), offset);
            const auto bucket_end      = std::next(bucket_begin, size);
            std::transform(bucket_begin, bucket_end, std::back_inserter(*result), [&](const BufferedStringView& sv) {
                return WordCount{sv.data, sv.len, count};
            });
        }
        return result;
    }

private:
    robin_hood::unordered_flat_map< int, std::array< unsigned, 2 > > m_bucket_info; // 0: bucket offset; 1: bucket size
    std::vector< int >                                               m_counts;
};

auto wordcount(const std::string& filePath) -> WordCountVec
{
    auto          file = MappedFile{filePath};
    std::optional dict{makeDictionary(file.getContents())};
    auto          bucket_manager = BucketManager{*dict};
    auto          bucket_vec     = bucket_manager.makeBuckets(*dict);
    bucket_manager.sortBuckets(bucket_vec);
    dict.reset();
    return bucket_manager.makeResult(bucket_vec, std::move(file));
}

#else
// Baseline solution.
// Do not change it - you can use for quickly checking speedups
// of your solution agains the baseline, see check_speedup.py
std::vector< WordCount > wordcount(std::string filePath)
{
    std::unordered_map< std::string, int > m;
    m.max_load_factor(0.5);

    std::vector< WordCount > mvec;

    std::ifstream inFile{filePath};
    if (!inFile)
    {
        std::cerr << "Invalid input file: " << filePath << "\n";
        return mvec;
    }

    std::string s;
    while (inFile >> s)
        m[s]++;

    mvec.reserve(m.size());
    for (auto& p : m)
        mvec.emplace_back(WordCount{p.second, move(p.first)});

    std::sort(mvec.begin(), mvec.end(), std::greater< WordCount >());
    return mvec;
}
#endif
