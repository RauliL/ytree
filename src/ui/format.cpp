#include "ui/format.hpp"

#include "core/api.hpp"

#include <format>

std::string FormatStatNumber(std::int64_t number)
{
  const auto number_separator = GetNumberSeparator();
  const auto terra =
    static_cast<long>(number / static_cast<std::int64_t>(1000000000000));
  const auto giga = static_cast<long>(
    (number % static_cast<std::int64_t>(1000000000000)) /
    static_cast<std::int64_t>(1000000000)
  );
  const auto mega = static_cast<long>(
    (number % static_cast<std::int64_t>(1000000000)) /
    static_cast<std::int64_t>(1000000)
  );
  const auto kilo = static_cast<long>(
    (number % static_cast<std::int64_t>(1000000)) /
    static_cast<std::int64_t>(1000)
  );
  const auto one = static_cast<long>(number % static_cast<std::int64_t>(1000));

  if (terra)
  {
    return std::format("[{:3}{:3}{:03}{:03}{:03}]", terra, giga, mega, kilo, one);
  }
  if (giga)
  {
    return std::format(
      "[{:3}{}{:03}{}{:03}{}{:03}]",
      giga,
      number_separator,
      mega,
      number_separator,
      kilo,
      number_separator,
      one
    );
  }
  if (mega)
  {
    return std::format(
      "[    {:3}{}{:03}{}{:03}]",
      mega,
      number_separator,
      kilo,
      number_separator,
      one
    );
  }
  if (kilo)
  {
    return std::format("[        {:3}{}{:03}]", kilo, number_separator, one);
  }
  return std::format("[            {:3}]", one);
}
