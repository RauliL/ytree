#pragma once

#include <functional>

class ScopeGuard
{
public:
  using callback_type = std::function<void()>;

  explicit ScopeGuard(const callback_type& callback)
    : callback_(std::move(callback)) {}

  ScopeGuard(ScopeGuard&& other) noexcept
    : callback_(std::move(other.callback_))
    , engaged_(other.engaged_)
  {
    other.engaged_ = false;
  }

  ~ScopeGuard()
  {
    if (engaged_)
    {
      callback_();
    }
  }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;

  void release()
  {
    engaged_ = false;
  }

private:
  callback_type callback_;
  bool engaged_{true};
};
