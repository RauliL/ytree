#include "ytree.h"

#include <regex>

static std::optional<std::regex> re;

int SetMatchSpec(const std::string& new_spec)
{
  bool meta_flag = false;
  std::string result;

  result.append(1, '^');

  for (const auto& c : new_spec)
  {
    if (meta_flag)
    {
      result.append(1, c);
      meta_flag = false;
      continue;
    }
    switch (c)
    {
      case '\\':
        meta_flag = true;
        break;

      case '?':
        result.append(1, '.');
        break;

      case '.':
        result.append(1, '\\');
        result.append(1, c);
        break;

      case '*':
        result.append(1, '.');
        result.append(1, '*');
        break;

      default:
        result.append(1, c);
        break;
    }
  }

  result.append(1, '$');

  try
  {
    re = std::regex(result, std::regex_constants::ECMAScript);
  }
  catch(const std::regex_error&)
  {
    return 1;
  }

  return 0;
}

bool Match(const std::string& filename)
{
  return re && std::regex_match(filename, *re);
}
