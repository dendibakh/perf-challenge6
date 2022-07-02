#ifndef WORDCOUNT_BUFFEREDSTRINGVIEW_HPP
#define WORDCOUNT_BUFFEREDSTRINGVIEW_HPP

#include <immintrin.h>

#include <array>
#include <cstring>
#include <string_view>

struct BufferedStringView
{
    static constexpr std::size_t buf_size = 28;
    using buf_t                           = std::array< char, buf_size >;

    [[nodiscard]] std::string_view viewPastBuf() const { return std::string_view{data + buf_size, len - buf_size}; }

    unsigned    len;
    buf_t       buf;
    const char* data;
};

template < typename Trgt, typename Src >
Trgt bit_cast(Src s)
{
    static_assert(sizeof(Src) == sizeof(Trgt));
    Trgt t;
    std::memcpy(&t, &s, sizeof(Trgt));
    return t;
}

inline auto safeCtz(unsigned a) -> int
{
    return a ? __builtin_ctz(a) : 32; // avoids UB for a == 0, but compiles down to tzcnt (no branch)
}

inline auto loadBuf(const BufferedStringView::buf_t& a) -> __m256i
{
    static_assert(BufferedStringView::buf_size <= 32);
    static_assert(BufferedStringView::buf_size % 4 == 0);

    constexpr int ones = -1, zeros = 0;
    const auto    load_mask = _mm256_setr_epi32(0 < BufferedStringView::buf_size ? ones : zeros,
                                             4 < BufferedStringView::buf_size ? ones : zeros,
                                             8 < BufferedStringView::buf_size ? ones : zeros,
                                             12 < BufferedStringView::buf_size ? ones : zeros,
                                             16 < BufferedStringView::buf_size ? ones : zeros,
                                             20 < BufferedStringView::buf_size ? ones : zeros,
                                             24 < BufferedStringView::buf_size ? ones : zeros,
                                             28 < BufferedStringView::buf_size ? ones : zeros);
    return _mm256_maskload_epi32(reinterpret_cast< const int* >(a.data()), load_mask);
}

inline auto bufCompareEqual(const BufferedStringView::buf_t& a1, const BufferedStringView::buf_t& a2) -> bool
{
    const auto v1         = loadBuf(a1);
    const auto v2         = loadBuf(a2);
    const auto cmp_result = _mm256_cmpeq_epi8(v1, v2);
    return not ~_mm256_movemask_epi8(cmp_result);
}

inline auto bufCompareLess(const BufferedStringView::buf_t& a1, const BufferedStringView::buf_t& a2) -> bool
{
    const auto v1         = loadBuf(a1);
    const auto v2         = loadBuf(a2);
    const auto eq_result  = _mm256_cmpeq_epi8(v1, v2);
    const auto leq_result = _mm256_cmpeq_epi8(_mm256_min_epu8(v1, v2), v1);
    const auto neq_mask   = ~_mm256_movemask_epi8(eq_result);
    const auto gt_mask    = ~_mm256_movemask_epi8(leq_result);
    const auto neq_ind    = safeCtz(bit_cast< unsigned >(neq_mask));
    const auto gt_ind     = safeCtz(bit_cast< unsigned >(gt_mask));
    return gt_ind > neq_ind;
}
#endif // WORDCOUNT_BUFFEREDSTRINGVIEW_HPP
