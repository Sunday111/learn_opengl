#pragma once

class Memory {
 public:
  static void* AlignedAlloc(size_t size, size_t alignment);
  static void* AlignedRealloc(void* ptr, size_t new_size, size_t alignment);
  static void AlignedFree(void* ptr);
};