#include "config.hpp"
#include "core/api.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

#if defined(WITH_UTF8)
# include <cwchar>
#endif

std::string StrLeft(const char* str, std::size_t count)
{
#if defined(WITH_UTF8)
  std::mbstate_t state{};
#endif
  std::size_t len = 0;

  if (count == 0)
  {
    return {};
  }

  const auto visual_len = static_cast<std::size_t>(StrVisualLength(str));
  if (count >= visual_len)
  {
    return str;
  }

#if defined(WITH_UTF8)
  const char* p = str;
  for (std::size_t i = 0; i < count; ++i)
  {
    const auto n = std::mbrlen(p, 4, &state);
    if (n == static_cast<std::size_t>(-1) || n == static_cast<std::size_t>(-2) || n == 0)
    {
      break;
    }
    len += n;
    p += n;
  }
#else
  len = count;
#endif

  return { str, len };
}

int StrVisualLength(std::string_view str)
{
  const std::string tmp(str);
  const char* cstr = tmp.c_str();

#if defined(WITH_UTF8)
  std::mbstate_t state{};
  auto len = static_cast<int>(std::mbsrtowcs(nullptr, &cstr, tmp.size(), &state));
  if (len < 0)
  {
    len = static_cast<int>(tmp.size());
  }

  return len;
#else
  return static_cast<int>(tmp.size());
#endif
}

const char* StrVisualIndex(const char* str, std::size_t index)
{
#if defined(WITH_UTF8)
  std::mbstate_t state{};
  const char* p = str;

  for (std::size_t i = 0; i < index && *p; ++i)
  {
    const auto n = std::mbrlen(p, 4, &state);
    if (n == static_cast<std::size_t>(-1) || n == static_cast<std::size_t>(-2) || n == 0)
    {
      break;
    }
    p += n;
  }

  return p;
#else
  const auto len = std::strlen(str);
  return str + std::min(index, len);
#endif
}

std::string FitVisualWidth(std::string_view str, std::size_t width, bool left_justify)
{
  const auto len = static_cast<std::size_t>(StrVisualLength(str));

  if (len > width)
  {
    return StrLeft(std::string(str).c_str(), width);
  }

  const auto pad = width - len;
  if (pad == 0)
  {
    return std::string(str);
  }

  if (left_justify)
  {
    return std::string(str) + std::string(pad, ' ');
  }

  return std::string(pad, ' ') + std::string(str);
}

void TruncateVisual(char* str, std::size_t max_len)
{
  if (static_cast<std::size_t>(StrVisualLength(str)) <= max_len)
  {
    return;
  }

  const auto truncated = StrLeft(str, max_len);
  std::memcpy(str, truncated.c_str(), truncated.size() + 1);
}
