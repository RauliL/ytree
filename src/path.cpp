#include "ytree.h"

#if defined(_WIN32)
const char* FILE_SEPARATOR_STRING = "\\";
#else
const char* FILE_SEPARATOR_STRING = "/";
#endif
const char FILE_SEPARATOR_CHAR = FILE_SEPARATOR_STRING[0];

std::optional<std::string> GetHomePath()
{
  const auto env_variable = std::getenv("HOME");

  if (!env_variable)
  {
    const auto pw = getpwuid(getuid());

    return pw ? std::make_optional<std::string>(pw->pw_dir) : std::nullopt;
  }

  return env_variable;
}

std::string PathJoin(const std::initializer_list<std::string>& parts)
{
  std::string result;

  for (const auto& part : parts)
  {
    if (!result.empty() && result[result.length() - 1] != FILE_SEPARATOR_CHAR)
    {
      result.append(1, FILE_SEPARATOR_CHAR);
    }
    result.append(part);
  }

  return result;
}

static std::optional<std::string> GetXdgPath(
  const char* env_variable_name,
  const char* directory_name
)
{
  if (const auto env_variable_value = std::getenv(env_variable_name))
  {
    return PathJoin({ env_variable_value, "ytree" });
  }
  else if (const auto home_path = GetHomePath())
  {
    return PathJoin({ *home_path, directory_name, "ytree" });
  }

  return std::nullopt;
}

std::optional<std::string> GetXdgConfigPath()
{
  return GetXdgPath("XDG_CONFIG_HOME", ".config");
}

std::optional<std::string> GetXdgCachePath()
{
  return GetXdgPath("XDG_CACHE_HOME", ".cache");
}
