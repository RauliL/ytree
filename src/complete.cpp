#include "complete.h"
#include "tilde.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace fs = std::filesystem;

char* DuplicateCString(const std::string& s)
{
  auto* copy = static_cast<char*>(std::malloc(s.size() + 1));
  if (!copy)
  {
    return nullptr;
  }
  std::memcpy(copy, s.c_str(), s.size() + 1);
  return copy;
}

std::string LongestCommonPrefix(const std::vector<std::string>& matches)
{
  if (matches.empty())
  {
    return {};
  }

  std::string prefix = matches.front();
  for (std::size_t i = 1; i < matches.size(); ++i)
  {
    const auto& candidate = matches[i];
    const auto n = std::min(prefix.size(), candidate.size());
    std::size_t j = 0;
    while (j < n && prefix[j] == candidate[j])
    {
      ++j;
    }
    prefix.resize(j);
    if (prefix.empty())
    {
      break;
    }
  }
  return prefix;
}

} // namespace

void free_completion_matches(char** matches)
{
  if (!matches)
  {
    return;
  }

  for (char** p = matches; *p; ++p)
  {
    std::free(*p);
  }
  std::free(matches);
}

char** filename_completion_matches(const std::string& text)
{
  const auto expanded = tilde_expand(text);

  std::string dir_part;
  std::string name_prefix;
  std::string display_prefix;

  const auto sep = expanded.find_last_of('/');
  if (sep == std::string::npos)
  {
    dir_part = ".";
    name_prefix = expanded;
    display_prefix.clear();
  }
  else
  {
    dir_part = expanded.substr(0, sep);
    if (dir_part.empty())
    {
      dir_part = "/";
    }
    name_prefix = expanded.substr(sep + 1);
    display_prefix = expanded.substr(0, sep + 1);
  }

  std::error_code ec;
  if (!fs::is_directory(dir_part, ec))
  {
    return nullptr;
  }

  std::vector<std::string> matches;
  for (fs::directory_iterator it(dir_part, ec), end; !ec && it != end;
       it.increment(ec))
  {
    const auto name = it->path().filename().string();
    if (name == "." || name == "..")
    {
      continue;
    }
    if (name.compare(0, name_prefix.size(), name_prefix) != 0)
    {
      continue;
    }

    auto entry = display_prefix + name;
    std::error_code entry_ec;
    if (fs::is_directory(it->path(), entry_ec))
    {
      entry.push_back('/');
    }
    matches.push_back(std::move(entry));
  }

  if (matches.empty())
  {
    return nullptr;
  }

  std::sort(matches.begin(), matches.end());

  const auto common = LongestCommonPrefix(matches);

  /* matches[0] = common prefix, matches[1..] = entries, then nullptr */
  auto** result = static_cast<char**>(
    std::malloc(sizeof(char*) * (matches.size() + 2))
  );
  if (!result)
  {
    return nullptr;
  }

  result[0] = DuplicateCString(common);
  if (!result[0])
  {
    std::free(result);
    return nullptr;
  }

  for (std::size_t i = 0; i < matches.size(); ++i)
  {
    result[i + 1] = DuplicateCString(matches[i]);
    if (!result[i + 1])
    {
      free_completion_matches(result);
      return nullptr;
    }
  }
  result[matches.size() + 1] = nullptr;
  return result;
}
