#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

void ReadProfile(const std::optional<std::filesystem::path>& custom_path);

std::optional<std::string> GetProfileValue(std::string_view key);

inline std::string GetProfileValueOrEmpty(std::string_view key)
{
  return GetProfileValue(key).value_or(std::string{});
}

double GetDoubleProfileValue(std::string_view key);

int GetIntProfileValue(std::string_view key);

bool GetBooleanProfileValue(std::string_view key);

char GetNumberSeparator();
