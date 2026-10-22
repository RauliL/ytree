#pragma once

#include <string>

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
