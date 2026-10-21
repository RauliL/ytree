#include <algorithm>
#include <cstring>

#include "core/ui.hpp"
#include "core/api.hpp"
#include "core/path.hpp"

static bool DeleteSubTree(DirEntry* dir_entry);
static bool DeleteSingleDirectory(DirEntry* dir_entry);
static void UnlinkDirEntry(DirEntry* dir_entry);

bool DirEntry::Delete()
{
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    ui::Beep();

    return false;
  }

  ui::ClearHelp();

  if (this == statistic.tree.get())
  {
    Message("Can't delete ROOT");
  }
  else if (!files.empty() || !children.empty())
  {
    if (Confirm("Directory not empty, PRUNE?"))
    {
      if (!children.empty())
      {
        ScanSubTree(this);
        if (!DeleteSubTree(this))
        {
          return false;
        }
      }
      if (!DeleteSingleDirectory(this))
      {
        return false;
      }

      return true;
    }
  }
  else if (Confirm("Delete this directory?"))
  {
    const auto path = GetPath();
    std::error_code ec;

    if (!std::filesystem::remove(path, ec))
    {
      FormatMessage(
        "Can't delete directory*\"{}\"*{}",
        path.string(),
        ec.message()
      );
    } else {
      // Directory geloescht => aus Baum loeschen
      --statistic.disk_total_directories;
      UnlinkDirEntry(this);
      GetAvailBytes(statistic.disk_space);

      return true;
    }
  }

  return false;
}

/**
 * Loescht alle Unterverzeichnisse von dir_entry.
 */
static bool DeleteSubTree(DirEntry* dir_entry)
{
  // Kopie, da DeleteSingleDirectory() die Eintraege austraegt
  const auto children = dir_entry->children;

  for (const auto& de_sp : children)
  {
    const auto de_ptr = de_sp.get();

    if (!de_ptr->children.empty())
    {
      if (!DeleteSubTree(de_ptr))
      {
        return false;
      }
    }
    if (!DeleteSingleDirectory(de_ptr))
    {
      return false;
    }
  }

  return true;
}

static bool DeleteSingleDirectory(DirEntry* dir_entry)
{
  const auto path = GetPath(dir_entry);
  std::error_code ec;

  if (!IsWriteable(path))
  {
    FormatMessage(
      "Can't delete directory*\"{}\"*{}",
      path.c_str(),
      std::strerror(errno)
    );

    return false;
  }

  // Kopie, da DeleteFile() die Eintraege austraegt
  const auto files = dir_entry->files;

  for (const auto& fe_ptr : files)
  {
    if (!fe_ptr->Delete())
    {
      return false;
    }
  }

  if (!std::filesystem::remove(path, ec))
  {
    FormatMessage(
      "Can't delete directory*\"{}\"*{}",
      path.string(),
      ec.message()
    );

    return false;
  }

  const auto parent = dir_entry->Parent();

  if (parent && !parent->not_scanned)
  {
    --statistic.disk_total_directories;
  }

  UnlinkDirEntry(dir_entry);

  return true;
}

/**
 * Traegt dir_entry aus der Liste der Unterverzeichnisse seines Vaters aus.
 */
static void UnlinkDirEntry(DirEntry* dir_entry)
{
  const auto parent = dir_entry->Parent();

  if (!parent)
  {
    return;
  }

  // The parent no longer owns the entry afterwards; it is destroyed here
  // unless something else (e.g. the dir window list) still shares it.
  auto& siblings = parent->children;

  siblings.erase(
    std::remove_if(
      std::begin(siblings),
      std::end(siblings),
      [dir_entry](const std::shared_ptr<DirEntry>& d)
      {
        return d.get() == dir_entry;
      }
    ),
    std::end(siblings)
  );
}
