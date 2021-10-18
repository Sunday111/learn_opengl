#pragma once

#include <filesystem>
#include <limits>
#include <memory>
#include <string_view>

#include "integer.hpp"

class Texture {
 public:
  Texture();
  ~Texture();
  static std::shared_ptr<Texture> LoadFrom(
      std::string_view path, const std::filesystem::path& src_dir);

  static inline ui32 kInvalidHandle = std::numeric_limits<ui32>::max();

  [[nodiscard]] inline bool IsValid() const noexcept {
    return handle_ != kInvalidHandle;
  }

  [[nodiscard]] ui32 GetHandle() const noexcept { return handle_; }

 private:
  ui32 handle_ = kInvalidHandle;
};
