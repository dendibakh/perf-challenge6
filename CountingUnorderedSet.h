#ifndef COUNTING_UNORDERED_SET_H
#define COUNTING_UNORDERED_SET_H
#include "wordcount.hpp"

#include <cstdint>
#include <list>
#include <vector>
#include <cassert>
#include <numeric>

#define XXH_INLINE_ALL
#include "xxhash.h"

class WordEntry {
private:
    static constexpr int MAX_SSO_SIZE = 8;
    union {
        struct {
            uint64_t _pad_hasList    : 1;
            uint64_t _pad_hasSso     : 1;
            uint64_t _pad1           : 6;
            uint64_t m_idx          : 33;
            uint64_t m_size         : 17;
            uint64_t padA           : 6;
        } __attribute__((packed));

        struct {
            uint8_t _pad2_hasList  : 1;
            uint8_t m_hasSso       : 1;
            uint8_t m_ssoSize      : 6;
            char    m_ssoData[MAX_SSO_SIZE-1];
        } __attribute__((packed));
    } __attribute__((packed));

    // clang is bad at packing "not full" bitfields. So this is dirty hack to have actually 8 bytes for SSO
    uint32_t _ssoAux : 8;
    uint32_t m_count : 24;
    static char* s_basePtr;
public:


    __attribute__((always_inline))
    inline const char* getPtr() const {
        if (m_hasSso) {
            return m_ssoData;
        } else {
            return &s_basePtr[m_idx];
        }
    }

    static const char* getPtr(uint64_t idx) {
        return &s_basePtr[idx];
    }

    __attribute__((always_inline))
    inline uint32_t getSize() const {
        if (m_hasSso) {
            return m_ssoSize;
        } else {
            return m_size;
        }
    }

    __attribute__((always_inline))
    inline uint32_t getCount() const {
        return m_count;
    }

    __attribute__((always_inline))
    inline bool isEmpty() const {
        return m_count == 0;
    }

    __attribute__((always_inline))
    inline bool hasSso() const {
        return m_hasSso;
    }

    __attribute__((always_inline))
    inline void increaseCount()  {
        m_count++;
    }

    __attribute__((always_inline))
    inline void resetCount()  {
        m_count = 0;
    }

    __attribute__((always_inline))
    inline bool equal(uint64_t idx, uint32_t size) const {
        if(m_hasSso) {
            return (m_ssoSize == size)
                    && (memcmp(m_ssoData, getPtr(idx), size) == 0);
        } else {
            return (m_size == size)
                    && (memcmp(&s_basePtr[m_idx], getPtr(idx), size) == 0);
        }
    }

    __attribute__((always_inline))
    inline void build(uint64_t idx, uint32_t size) {
        if (size <= MAX_SSO_SIZE) {
            m_hasSso = true;
            m_ssoSize = size;
            memcpy(m_ssoData, getPtr(idx), MAX_SSO_SIZE);
        } else {
            m_idx = idx;
            m_size = size;
        }
    }

    __attribute__((always_inline))
    inline void build(uint64_t idx, uint32_t size, uint32_t count) {
        build(idx, size);
        m_count = count;
    }

    __attribute__((always_inline))
    inline void prefetch() const {
        if (!isEmpty() && !hasSso()) {
            __builtin_prefetch(getPtr());
        }
    }



    static void setBasePtr(char* basePtr) {
        s_basePtr = basePtr;
    }

    void operator=(const WordEntry& other) {
        memcpy(this, &other, sizeof(*this));
    }
} __attribute__((packed));


template<uint8_t POW2_SIZE, uint32_t COLLISION_BUCKETS>
class CountingUnorderedSet{
    static constexpr uint32_t NUM_BUCKETS = 1 << POW2_SIZE;
    static constexpr uint32_t HASH_MASK = NUM_BUCKETS - 1;
    static constexpr uint32_t NUM_SHORT_WORDS = 1 << 16;

public:
    struct ListEntry {
        WordEntry entry;
        uint32_t m_nextIdx;

        uint32_t getNextIdx() const {
            return m_nextIdx;
        }

        void addList(uint32_t nextIdx) {
            m_nextIdx = nextIdx;
        }
    };

    struct Bucket {
        union {
            WordEntry entry;
            struct {
                uint32_t m_hasList : 1;
                uint32_t m_nextIdx  : 31;
            } __attribute__((packed));
        } __attribute__((packed));

        bool hasList() const {
            return m_hasList;
        }
        bool hasNext() const {
            return (m_nextIdx != 0);
        }

        void addList(uint32_t nextIdx) {
            m_hasList = true;
            m_nextIdx = nextIdx;
            entry.resetCount();
        }

        uint32_t getNextIdx() const {
            return m_nextIdx;
        }
    } __attribute__((packed));

    CountingUnorderedSet() :
    m_size(0),
    m_collisionIdx(1) {
    }

    //TODO: null check needed?
    __attribute__((always_inline))
    inline ListEntry* getNext(const Bucket* bucket) {
        return &m_collisionBuckets[bucket->getNextIdx()];
    }

    __attribute__((always_inline))
    inline const ListEntry* getNext(const Bucket* bucket) const {
        return &m_collisionBuckets[bucket->getNextIdx()];
    }

    __attribute__((always_inline))
    inline ListEntry* getNext(const ListEntry* listEntry) {
        return (listEntry->getNextIdx() == 0) ? nullptr : &m_collisionBuckets[listEntry->getNextIdx()];
    }

    __attribute__((always_inline))
    inline const ListEntry* getNext(const ListEntry* listEntry) const {
        return (listEntry->getNextIdx() == 0) ? nullptr : &m_collisionBuckets[listEntry->getNextIdx()];
    }

    __attribute__((always_inline))
    inline void increaseCount(ListEntry* listEntry) {
        listEntry->entry.increaseCount();
    }

    __attribute__((always_inline))
    inline void increaseCount(Bucket* bucket) {
        bucket->entry.increaseCount();
    }

    __attribute__((always_inline))
    inline Bucket* getBucket(uint64_t idx, uint32_t size) {
        return &buckets[hash(idx, size) & HASH_MASK];
    }

    __attribute__((always_inline))
    inline void add(uint64_t idx, uint32_t size) {
        addWithHint(getBucket(idx, size), idx, size);
    }

    __attribute__((always_inline))
    inline void addWithHint(Bucket* bucket, uint64_t idx, uint32_t size) {
        if (bucket->hasList()) {
            addToList(bucket, idx, size);
        } else {
            if(bucket->entry.getCount() > 0) {
                if (bucket->entry.equal(idx, size)) {
                    increaseCount(bucket);
                } else {
                    ListEntry* listEntry = moveEntryFromBucketToList(bucket);

                    listEntry->addList(m_collisionIdx++);
                    listEntry = getNext(listEntry);
                    addNew(listEntry, idx, size);
                }
            } else {
                addNew(bucket, idx, size);
            }
        }
    }

    __attribute__((always_inline))
    inline void addFast(uint16_t c) {
        m_shortCounters[c]++;
    }

    ListEntry* moveEntryFromBucketToList(Bucket* bucket) {
        auto newListEntryIdx = m_collisionIdx++;
        ListEntry* listEntry = &m_collisionBuckets[newListEntryIdx];
        listEntry->entry = bucket->entry;
        bucket->addList(newListEntryIdx);

        return listEntry;
    }

    __attribute__((always_inline))
    void addNew(Bucket* bucket, uint64_t idx, uint32_t size) {
        bucket->entry.build(idx, size, 1);
        m_size++;
    }

    __attribute__((always_inline))
    void addNew(ListEntry* listEntry, uint64_t idx, uint32_t size) {
        listEntry->entry.build(idx, size, 1);
        m_size++;
    }

    __attribute__((always_inline))
    inline void addToList(Bucket* bucket, uint64_t idx, uint32_t size) {
        addToList(getNext(bucket), idx, size);
    }

    __attribute__((always_inline))
    inline void addToList(ListEntry* listEntry, uint64_t idx, uint32_t size) {
        if (listEntry->entry.equal(idx, size)) {
            increaseCount(listEntry);
            return;
        }

        do {
            listEntry = getNext(listEntry);
            if (listEntry->entry.equal(idx, size)) {
                increaseCount(listEntry);
                return;
            }
        } while (getNext(listEntry) != nullptr);

        listEntry->addList(m_collisionIdx++);
        listEntry = getNext(listEntry);
        addNew(listEntry, idx, size);
    }

    __attribute__((always_inline))
    inline void prefetch(const Bucket* bucket) const {
        if(bucket->hasList())
            __builtin_prefetch(getNext(bucket));
        else if (!bucket->entry.isEmpty() && !bucket->entry.hasSso()) {
            __builtin_prefetch(bucket->entry.getPtr());
        }
    }

    size_t size() const{
        return m_size;
    }

    void getVec(std::vector<WordCount>& mvec) const {
        const auto realSize = m_size + NUM_SHORT_WORDS;
        mvec.reserve(realSize);

        int i = 0;
        for(; i<256; i++) {
            if(m_shortCounters[i] > 0) {
                mvec.emplace_back(WordCount{(int)m_shortCounters[i], move(std::string(1, (char)i))});
            }
        }

        for(; i<NUM_SHORT_WORDS; i++) {
            const uint16_t c = i;
            if(m_shortCounters[i] > 0) {
                mvec.emplace_back(WordCount{(int)m_shortCounters[i], move(std::string((char*)&c, 2))});
            }
        }

        const int READ_AHEAD = 16;
        for(auto i=0; i<NUM_BUCKETS; i++) {
            if (i < NUM_BUCKETS - READ_AHEAD) {
                if (!buckets[i + READ_AHEAD].entry.hasSso() && !buckets[i + READ_AHEAD].entry.isEmpty())
                    __builtin_prefetch(buckets[i + READ_AHEAD].entry.getPtr());
            }

            auto& bucket = buckets[i];
            auto& countedEntry = bucket.entry;
            if (!countedEntry.isEmpty())
                mvec.emplace_back(WordCount{(int)countedEntry.getCount(), move(std::string(countedEntry.getPtr(), countedEntry.getSize()))});
        }

        for(auto i=1; i<m_collisionIdx; i++) {
            if (i < m_collisionIdx - READ_AHEAD) {
                if (!m_collisionBuckets[i + READ_AHEAD].entry.hasSso() && !m_collisionBuckets[i + READ_AHEAD].entry.isEmpty())
                    __builtin_prefetch(m_collisionBuckets[i + READ_AHEAD].entry.getPtr());
            }
            auto& countedEntry = m_collisionBuckets[i].entry;
            mvec.emplace_back(WordCount{(int)countedEntry.getCount(), move(std::string(countedEntry.getPtr(), countedEntry.getSize()))});
        }
        assert(realSize == mvec.size());
    }

    void printStats() const {
        uint32_t maxCollisions = 0;
        uint32_t maxWordSize = 0;
        uint32_t maxWordCount = 0;
        uint32_t totalSso = 0;
        uint32_t totalShort = std::accumulate(m_shortCounters, m_shortCounters + NUM_SHORT_WORDS, 0ul);
        uint64_t totalProcessed = std::accumulate(m_shortCounters, m_shortCounters + NUM_SHORT_WORDS, 0ul);

        for(auto i=0; i<NUM_BUCKETS; i++) {
            const Bucket* bucket = &buckets[i];
            if (bucket->hasList()) {
                uint32_t collisions = 0;
                const ListEntry* listEntry = getNext(bucket);
                do {
                    maxWordSize = std::max(maxWordSize, listEntry->entry.getSize());
                    maxWordCount = std::max(maxWordCount, listEntry->entry.getCount());
                    totalProcessed += listEntry->entry.getCount();
                    if (listEntry->entry.hasSso()) totalSso++;
                    collisions++;
                } while((listEntry = getNext(listEntry)) != nullptr);
                maxCollisions = std::max(maxCollisions, collisions);
            } else {
                maxWordSize = std::max(maxWordSize, bucket->entry.getSize());
                maxWordCount = std::max(maxWordCount, bucket->entry.getCount());
                totalProcessed += bucket->entry.getCount();
                if (bucket->entry.hasSso()) totalSso++;
            }
        }
        printf("Sizes WordEntry: %llu, Bucket: %llu ListEntry: %llu\n", sizeof(WordEntry), sizeof(Bucket), sizeof(ListEntry));
        printf("CountingUnorderedSet size: %zu. max collision chain: %u. Total collisions: %zu. Max word size: %u Max word count: %u. Total Sso: %u. Total short: %u Total processed: %zu\n", size(),  maxCollisions, m_collisionIdx, maxWordSize, maxWordCount, totalSso, totalShort, totalProcessed);
    }



private:
    size_t m_size;
    Bucket buckets[NUM_BUCKETS];
    size_t m_collisionIdx;
    ListEntry m_collisionBuckets[COLLISION_BUCKETS];
    uint32_t m_shortCounters[NUM_SHORT_WORDS];

    __attribute__((always_inline))
    inline size_t hash(uint64_t idx, uint32_t size) const {
        auto ptr = WordEntry::getPtr(idx);
        return XXH3_64bits(ptr, size);
    }
};

#endif /* COUNTING_UNORDERED_SET_H */