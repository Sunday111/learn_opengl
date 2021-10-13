#pragma once

#include <string_view>

#include "integer.hpp"

class Name {
 public:
  using NameId = ui32;
  static inline constexpr NameId kInvalidNameId =
      std::numeric_limits<NameId>::max();

 public:
  Name() = default;
  Name(const std::string_view& view);

  std::string_view GetView() const;

  friend [[nodiscard]] inline bool operator==(const Name& a,
                                              const Name& b) noexcept {
    return a.id_ == b.id_;
  }

  friend [[nodiscard]] inline bool operator<(const Name& a,
                                             const Name& b) noexcept {
    return a.id_ < b.id_;
  }

 private:
  [[nodiscard]] bool IsValid() const noexcept;

 private:
  NameId id_ = kInvalidNameId;
};
