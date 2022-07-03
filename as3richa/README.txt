* Use C stdio (fread) with a large buffer, rather than reading individual strings with cin
* Use AVX2 SIMD comparison to identify and split on whitespace characters
* Replace std::unordered_map with a simple hand-rolled hashtable implementation, specialized for the specific operation of incrementing a counter
 - Power-of-two sizes
 - Linear probing
 - std::hash
* Allocate strings in a single packed arena
 - Address strings by an offset, len pair, both 32 bits
 - Create WordCountResult and WordCountEntry structures to encode the result using the arena
* Sort by a composite integer sort key, composed of the count and the first 4 bytes of the string, prior to sorting subranges by the full string
* Use mmap with hugepages for large allocations on Linux
 - If this causes allocation failures, can be disabled by commenting out `#define USE_MMAP_HUGE_TLB` in alloc.hpp