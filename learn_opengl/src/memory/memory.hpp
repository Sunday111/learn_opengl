#pragma once

#include <cstdint>
#include <cstddef>

class Memory {
 public:
  static void* AlignedAlloc(std::size_t size, std::size_t alignment);
  static void* AlignedRealloc(void* ptr, std::size_t new_size,
                              std::size_t alignment);
  static void AlignedFree(void* ptr);
};