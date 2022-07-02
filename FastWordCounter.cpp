#define _CRT_SECURE_NO_WARNINGS
#include "FastWordCounter.h"
#include "wordcount.hpp"
#include <array>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <chrono>

#define LOG(...) printf(__VA_ARGS__)
//#define LOG(...)

FastWordCounter::FastWordCounter() :
    m_wordCountingSet(new WordCountingSet),
    m_preloadedFile(nullptr),
    m_fileSize(0),
    m_totalDuration(0)
{
}

FastWordCounter::~FastWordCounter() {
    delete m_wordCountingSet;
    free(m_preloadedFile);
}

void FastWordCounter::run(std::string filePath, std::vector<WordCount>& mvec) {
    startTimer();
    allocate(filePath);
    endTimer("allocate");

    startTimer();
    tokenize(filePath);
    endTimer("tokenize");

    startTimer();
    buildVector(mvec);
    endTimer("buildVector");

    startTimer();
    sort(mvec);
    endTimer("sort");
    //printf("TOTAL: %lld ms\n", m_totalDuration);
}

void FastWordCounter::allocate(std::string filePath) {
    std::error_code ec;
    m_fileSize = std::filesystem::file_size(filePath, ec);

    m_preloadedFile = (char*)malloc(m_fileSize+1);
    assert(m_preloadedFile != nullptr);
    WordEntry::setBasePtr(m_preloadedFile);
}

__attribute__((always_inline))
inline bool isSeparator(char c) {
    return ((c <= ' ') && ((c == ' ') || (c == '\t') || (c == '\n')));
}

__attribute__((always_inline))
inline uint64_t FastWordCounter::getWordStart(uint64_t idx) const {
    while (isSeparator(m_preloadedFile[idx])) {
        idx++;
    }
    return idx;
}

__attribute__((always_inline))
inline uint64_t FastWordCounter::getWordEnd(uint64_t idx) const {
    while (!isSeparator(m_preloadedFile[idx])) {
        idx++;
    }
    return idx;
}

void FastWordCounter::tokenize(std::string filePath) {
    struct PrefetchingEntry {
        uint64_t wordIdx;
        uint32_t wordSize;
        WordCountingSet::Bucket* bucket;

        void build(uint64_t idx, uint32_t size) {
            wordIdx = idx;
            wordSize = size;
        }
    };
    std::array<PrefetchingEntry, READ_AHEAD> m_prefetchingCache;

    int64_t idx = 0;
    uint32_t cacheIdx = 0;
    int64_t totalRead = 0;
    uint64_t readCount = 0;

    const int64_t READ_CHUNK = 2 * 1024 * 1024;
    const uint32_t READ_MARGIN = 96 * 1024;
    FILE* stream = fopen(filePath.c_str(), "r");
    bool readingDone = false;

    int64_t nextPrefetch = 0;
    const int64_t PREFETCH_AHEAD = 512;
    do {
        if ((idx > totalRead - READ_MARGIN) && !readingDone) {
            uint64_t readCount = fread(m_preloadedFile + totalRead, sizeof(char), std::min(READ_CHUNK, m_fileSize - totalRead), stream);
            totalRead += readCount;

            if (totalRead == m_fileSize) {
                m_preloadedFile[m_fileSize] = '\n';
                // trim trailing spaces
                uint64_t idx = m_fileSize;
                while(isSeparator(m_preloadedFile[idx])) {
                    idx--;
                }
                m_fileSize = idx + 1;

                fclose(stream);
                readingDone = true;
            }
        }
        if (idx > nextPrefetch) {
            __builtin_prefetch(m_preloadedFile+idx+PREFETCH_AHEAD);
            __builtin_prefetch(m_preloadedFile+idx+PREFETCH_AHEAD+64);
            nextPrefetch = idx + 128;
        }
        auto startIdx = idx = getWordStart(idx);
        idx = getWordEnd(idx);
        const auto wordLen = idx - startIdx;
        if(wordLen == 1) {
            m_wordCountingSet->addFast(m_preloadedFile[startIdx]);
        } else if(wordLen == 2) {
            uint16_t val = *(uint16_t*)(m_preloadedFile+startIdx);
            m_wordCountingSet->addFast(val);
        }
        else {
            auto& cacheEntry = m_prefetchingCache[cacheIdx % READ_AHEAD];
            if (cacheIdx>=READ_AHEAD) {
                m_wordCountingSet->addWithHint(cacheEntry.bucket, cacheEntry.wordIdx, cacheEntry.wordSize);
            }
            // prefetch new cache entry
            cacheEntry.bucket = m_wordCountingSet->getBucket(startIdx, wordLen);
            //replace cache entry with new item
            cacheEntry.build(startIdx, wordLen);

            if (cacheIdx>=READ_AHEAD2) {
                auto& cacheEntry2 = m_prefetchingCache[(cacheIdx - READ_AHEAD2) % READ_AHEAD];
                m_wordCountingSet->prefetch(cacheEntry2.bucket);
            }
            __builtin_prefetch(cacheEntry.bucket);
            cacheIdx++;
        }
    } while(idx < m_fileSize);

    for(int i=0; i<READ_AHEAD; i++) {
        auto& cacheEntry = m_prefetchingCache[cacheIdx % READ_AHEAD];
        m_wordCountingSet->addWithHint(cacheEntry.bucket, cacheEntry.wordIdx, cacheEntry.wordSize);
        cacheIdx++;
    }
}

void FastWordCounter::buildVector(std::vector<WordCount>& mvec) {
    m_wordCountingSet->getVec(mvec);
}

void FastWordCounter::sort(std::vector<WordCount>& mvec) {
    std::sort(mvec.begin(), mvec.end(), std::greater<WordCount>());
}

void FastWordCounter::startTimer() {
    //m_startTime = std::chrono::high_resolution_clock::now();
}

void FastWordCounter::endTimer(const char* name) {
    /*auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_startTime).count();
    LOG("%s: %lld ms\n", name, duration);
    m_totalDuration += duration;*/
}
