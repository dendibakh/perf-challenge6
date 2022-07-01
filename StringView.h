#pragma once
#include <algorithm>
#include <cstring>
#include <immintrin.h>
#include <nmmintrin.h>
#include <string>

class StringView
{
    static const int SSO_SIZE = 16;

public:
    StringView() noexcept
        : length(0)
        , realBuffer(nullptr)
    {}

    explicit StringView(const char* buf, int len) noexcept
    {
        if (len <= SSO_SIZE) {
            memset(buffer, 0, SSO_SIZE);
            memcpy(buffer, buf, len);
            realBuffer = buffer;
        }
        else {
            realBuffer = buf;
        }
        length = len;
    }

    StringView(const StringView& other) noexcept
    {
        memcpy(this, &other, sizeof(StringView));
        if (size() <= SSO_SIZE)
            realBuffer = buffer;
    }

    StringView(StringView&& other) noexcept
    {
        memcpy(this, &other, sizeof(StringView));
        if (size() <= SSO_SIZE)
            realBuffer = buffer;
    }

    StringView& operator=(const StringView& other) noexcept
    {
        memcpy(this, &other, sizeof(StringView));
        if (size() <= SSO_SIZE)
            realBuffer = buffer;
        return *this;
    }

    StringView& operator=(StringView&& other) noexcept
    {
        memcpy(this, &other, sizeof(StringView));
        if (size() <= SSO_SIZE)
            realBuffer = buffer;
        return *this;
    }

    bool operator<(const StringView& other) const
    {
        if constexpr (SSO_SIZE == 16) {
            auto as = _mm_loadu_si128((__m128i*)data());
            auto bs = _mm_loadu_si128((__m128i*)other.data());

            int result = _mm_cmpistri(as, bs, _SIDD_NEGATIVE_POLARITY | _SIDD_CMP_EQUAL_EACH);

            bool retValue = false;
            if (result < 16) {
                retValue = (unsigned char)data()[result] < (unsigned char)other.data()[result];
                return retValue;
            }

            if(size() < 16 && other.size() < 16)
                return retValue;
        }
        return this->string_view() < other.string_view();
    }

    bool operator==(const StringView& other) const {
        //return this->string_view() != other.string_view();
        if (size() != other.size())
            return false;
        if constexpr (SSO_SIZE == 16) {
            if (size() <= SSO_SIZE) {
                auto a = _mm_lddqu_si128((const __m128i*)data());
                auto b = _mm_lddqu_si128((const __m128i*)other.data());

                auto neq = _mm_xor_si128(a, b);
                bool res = _mm_testz_si128(neq, neq);
                return res;
            }
        }
        if constexpr (SSO_SIZE == 32) {
            if (size() <= SSO_SIZE) {
                auto a = _mm256_lddqu_si256((const __m256i*)data());
                auto b = _mm256_lddqu_si256((const __m256i*)other.data());

                auto neq = _mm256_xor_si256(a, b);
                bool res = _mm256_testz_si256(neq, neq);
                return res;
            }
        }
        return memcmp(data(), other.data(), size()) == 0;
    }

    const char* data() const
    {
        return realBuffer;
    }

    int size() const
    {
        return length;
    }

    std::string_view string_view() const
    {
        return std::string_view( realBuffer, length );
    }

    std::string str() const
    {
        return std::string(string_view());
    }

private:
    char buffer[SSO_SIZE];
    int length;
    const char* realBuffer;
};

namespace std
{
    template <>
    struct hash<StringView>
    {
        std::size_t operator()(const StringView& c) const
        {
            std::size_t h = std::hash<std::string_view>()(c.string_view());
            return h;
        }
    };
}