#ifndef ALLOC_H
#define ALLOC_H

#include <cstdlib>

#ifdef __linux__
#define USE_MMAP_HUGE_TLB
#include <sys/mman.h>
#define aligned_free(ptr) free(ptr)
#endif

#ifdef _WIN32
#include <malloc.h>
#define aligned_alloc(alloc_alignment, len) _aligned_malloc(len, alloc_alignment)
#define aligned_free(ptr) _aligned_free(ptr)
#endif

static const size_t mmap_threshold = 2048;
static const size_t alloc_alignment = 32;

inline void *my_alloc(size_t len) {
    len += alloc_alignment - len % alloc_alignment;

#ifdef USE_MMAP_HUGE_TLB
    if (len >= mmap_threshold) {
        void *ptr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

        if (ptr == NULL || ptr == (void*)-1) {
            abort();
        }

        return ptr;
    }
#endif

    void *ptr = aligned_alloc(alloc_alignment, len);

    if (ptr == NULL) {
        abort();
    }

    memset(ptr, 0, len);

    return ptr;
}

inline void my_free(void *pointer, size_t len) {
    len += alloc_alignment - len % alloc_alignment;

#ifdef USE_MMAP_HUGE_TLB
    if (len >= mmap_threshold) {
        munmap(pointer, len);
        return;
    }
#endif

    aligned_free(pointer);
}

#endif