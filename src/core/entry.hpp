#pragma once

#include "core/entry-meta.hpp"

/**
 * Represents an file system entity.
 */
struct Entry
{
  std::string name;
  EntryMeta meta{};

  virtual ~Entry() = default;

  virtual std::filesystem::path GetPath() const = 0;

  bool ChangeModus();

  bool SetModus(const std::string& new_modus_str);

  bool ChangeOwner();

  bool SetOwner(int new_user_id);

  bool ChangeGroup();

  bool SetGroup(int new_group_id);

  virtual bool Delete() = 0;
};
