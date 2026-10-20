#pragma once

#include <optional>
#include <string>

/**
 * Splits string view on delimiters.
 */
class StringTokenizer
{
public:
  explicit StringTokenizer(std::string_view input) : rest_(input) {}

  /**
   * Next token. An empty `delimiters` string means "remainder of the line"
   * (filenames may contain spaces), matching classic Strtok_r(..., "").
   */
  std::optional<std::string_view> Next(std::string_view delimiters)
  {
    while (!rest_.empty() &&
           !delimiters.empty() &&
           delimiters.find(rest_.front()) != std::string_view::npos)
    {
      rest_.remove_prefix(1);
    }

    if (rest_.empty())
    {
      return std::nullopt;
    }

    if (delimiters.empty())
    {
      const auto token = rest_;

      rest_ = {};

      return token;
    }

    const auto pos = rest_.find_first_of(delimiters);
    std::string_view token;

    if (pos == std::string_view::npos)
    {
      token = rest_;
      rest_ = {};
    } else {
      token = rest_.substr(0, pos);
      rest_.remove_prefix(pos + 1);
    }

    return token;
  }

private:
  std::string_view rest_;
};
