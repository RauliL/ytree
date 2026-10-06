#include "ytree.h"

#include <filesystem>

#include <peelo/xdg.hpp>

std::optional<std::string> GetHomePath()
{
  if (const auto home = peelo::xdg::home_dir())
  {
    return home->string();
  }

  return std::nullopt;
}

std::string PathJoin(const std::initializer_list<std::string>& parts)
{
  std::filesystem::path result;

  for (const auto& part : parts)
  {
    result /= part;
  }

  return result.string();
}

std::optional<std::string> GetXdgConfigPath()
{
  if (const auto config_dir = peelo::xdg::config_dir())
  {
    return (*config_dir / "ytree").string();
  }

  return std::nullopt;
}

std::optional<std::string> GetXdgCachePath()
{
  if (const auto cache_dir = peelo::xdg::cache_dir())
  {
    return (*cache_dir / "ytree").string();
  }

  return std::nullopt;
}
