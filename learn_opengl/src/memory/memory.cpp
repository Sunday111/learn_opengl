#include "memory/memory.hpp"

#include <cstdlib>

/* C11 - The Universal CRT implemented the parts of the C11 Standard Library
 * that are required by C++17, with the exception of C99 strftime() E/O
 * alternative conversion specifiers, C11 fopen() exclusive mode, and C11
 * aligned_alloc(). The latter is unlikely to be implemented, because C11
 * specified aligned_alloc() in a way that's incompatible with the Microsoft
 * implementation of free(): namely, that free() must be able to handle highly
 * aligned allocations.
 */

void* Memory::AlignedAlloc(size_t size, [[maybe_unused]] size_t alignment) {
#ifdef _MSC_VER
  return _aligned_malloc(size, alignment);
#else
  // compiles but fires an exception...
  return std::aligned_alloc(size, alignment);
#endif
}

void* Memory::AlignedRealloc(void* ptr, size_t new_size,
                             [[maybe_unused]] size_t alignment) {
#ifdef _MSC_VER
  return _aligned_realloc(ptr, new_size, alignment);
#else
  return std::realloc(ptr, new_size);
#endif
}

void Memory::AlignedFree(void* ptr) {
#ifdef _MSC_VER
  return _aligned_free(ptr);
#else
  return std::free(ptr);
#endif
}