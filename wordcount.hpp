#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

// use it for init any object you may need.
void init();

#ifdef SOLUTION

#include "BufferedStringView.hpp"
#include "MappedFile.hpp"

#include <deque>
#include <memory>
#include <string_view>

// This is an ugly hack to let the validation function construct WordCounts without the underlying file
// This is not cheating - `wordcount` never accesses the global string dump in any way
inline auto getGlobalStringDump() -> std::deque< std::string >&
{
    static std::deque< std::string > value;
    return value;
}

struct WordCount
{
public:
    std::string_view word;
    int              count;

    WordCount(const char* data, unsigned size, int count_) : word{data, size}, count{count_} {}
    WordCount(int count_, std::string&& word_)
        : count{count_}, word{getGlobalStringDump().emplace_back(std::move(word_))}
    {}

    friend bool operator>(const WordCount& lh, const WordCount& rh)
    {
        if (lh.count > rh.count)
            return true;
        if (lh.count == rh.count)
            return lh.word < rh.word;
        return false;
    }

    friend bool operator!=(const WordCount& lh, const WordCount& rh)
    {
        return lh.count != rh.count || lh.word != rh.word;
    }
};

class WordCountVec
{
public:
    explicit WordCountVec(MappedFile&& file) : m_file{std::move(file)} {}

    [[nodiscard]] auto        begin() { return m_word_counts.begin(); }
    [[nodiscard]] auto        end() { return m_word_counts.end(); }
    [[nodiscard]] auto        begin() const { return m_word_counts.begin(); }
    [[nodiscard]] auto        end() const { return m_word_counts.end(); }
    [[nodiscard]] auto        size() const { return m_word_counts.size(); }
    [[nodiscard]] auto&       operator[](std::ptrdiff_t i) { return m_word_counts[i]; }
    [[nodiscard]] const auto& operator[](std::ptrdiff_t i) const { return m_word_counts[i]; }

    auto operator*() -> std::vector< WordCount >& { return m_word_counts; }
    auto operator->() -> std::vector< WordCount >* { return std::addressof(m_word_counts); }

private:
    std::vector< WordCount > m_word_counts;
    MappedFile               m_file;
};

auto wordcount(const std::string& fileName) -> WordCountVec;

#else

struct WordCount
{
    int         count;
    std::string word;

    friend bool operator>(const WordCount& lh, const WordCount& rh)
    {
        if (lh.count > rh.count)
            return true;
        if (lh.count == rh.count)
            return lh.word < rh.word;
        return false;
    }

    friend bool operator!=(const WordCount& lh, const WordCount& rh)
    {
        return lh.count != rh.count || lh.word != rh.word;
    }
};

std::vector< WordCount > wordcount(std::string fileName);

#endif