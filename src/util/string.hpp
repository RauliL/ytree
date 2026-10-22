#pragma once

#include <string>

template<class T = int>
inline int ToInt(std::string_view input)
{
  return static_cast<T>(std::strtol(
    std::string(input).c_str(),
    nullptr,
    10
  ));
}

template<class... Args>
std::string Join(Args&&... args)
{
  std::string result;
  bool first = true;
  auto append = [&result, &first](const std::string& part)
  {
    if (first)
    {
      first = false;
    } else {
      result.append(1, ' ');
    }
    result.append(part);
  };

  (append(args), ...);

  return result;
}
