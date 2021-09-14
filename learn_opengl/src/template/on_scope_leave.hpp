#pragma once

namespace __on_scope_leave_impl {
template <typename Function>
class OnScopeLeaveHandler {
 public:
  ~OnScopeLeaveHandler() { function_(); }
  Function function_;
};
}  // namespace __on_scope_leave_impl

template <typename Function>
auto OnScopeLeave(Function&& fn) {
  using namespace __on_scope_leave_impl;
  return OnScopeLeaveHandler{std::forward<Function>(fn)};
}