#pragma once

#include <cstdint>

enum class NodeKind : std::uint8_t
{
  /** Paths that are member names + volume extract/view. */
  Archive,
  /** Path that hit the real filesystem. */
  Disk,
  /** USER_MODE / custom action trees. */
  User,
};
