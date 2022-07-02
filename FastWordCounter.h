#ifndef FAST_WORD_COUNTER_H
#define FAST_WORD_COUNTER_H
#include "CountingUnorderedSet.h"
#include "wordcount.hpp"

#include <chrono>
#include <string>
#include <vector>


class FastWordCounter {
    public:
        FastWordCounter();
        ~FastWordCounter();
        void run(std::string filePath, std::vector<WordCount>& mvec);

    private:
        static constexpr int READ_AHEAD = 32;
        static constexpr int READ_AHEAD2 = READ_AHEAD / 2;

        void allocate(std::string filePath);
        void tokenize(std::string filePath);
        void buildVector(std::vector<WordCount>& mvec);
        void sort(std::vector<WordCount>& mvec);

        uint64_t getWordStart(uint64_t idx) const;
        uint64_t getWordEnd(uint64_t idx) const;

        void startTimer();
        void endTimer(const char* name);

        using WordCountingSet = CountingUnorderedSet<27, 10500000>; //28 = 6000000, 27=10500000
        WordCountingSet* m_wordCountingSet;
        char* m_preloadedFile;
        int64_t m_fileSize;

        std::chrono::high_resolution_clock::time_point m_startTime;
        uint64_t m_totalDuration;
};

#endif /* FAST_WORD_COUNTER_H */