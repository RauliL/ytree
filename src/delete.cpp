#include <algorithm>

#include "./ytree.hpp"

bool FileEntry::Delete()
{
  const auto path = GetPath();
  std::error_code ec;
  bool result;

  if (meta.IsLink() && !IsWriteable(path))
  {
    if (!std::filesystem::exists(path))
    {
      // Datei existiert nicht ==> fertig
      goto UNLINK_DONE;
    }

    if (!Confirm(
      std::format(
        "Overriding mode {:04o} for \"{}\"?",
        meta.Mode() & 0777,
        name
      )
    ))
    {
      FormatMessage(
        "Can't delete file*\"{}\"*{}",
        path.string(),
        std::strerror(errno)
      );

      return false;
    }
  }

  std::filesystem::remove(path, ec);
  if (ec)
  {
    FormatMessage("Can't delete file*\"{}\"*{}", path.string(), ec.message());

    return false;
  }

UNLINK_DONE:
  // File austragen
  result = Remove();
  GetAvailBytes(&statistic.disk_space);

  return result;
}

bool FileEntry::Remove()
{
  std::int64_t file_size;
  const auto de_ptr = Dir();

  if (!de_ptr)
  {
    return false;
  }

  file_size = meta.size;

  de_ptr->total_bytes -= file_size;
  --de_ptr->total_files;
  statistic.disk_total_bytes -= file_size;
  --statistic.disk_total_files;
  if (matching)
  {
    de_ptr->matching_bytes -= file_size;
    --de_ptr->matching_files;
    statistic.disk_matching_bytes -= file_size;
    --statistic.disk_matching_files;
  }
  if (tagged)
  {
    de_ptr->tagged_bytes -= file_size;
    --de_ptr->tagged_files;
    statistic.disk_tagged_bytes -= file_size;
    --statistic.disk_tagged_files;
  }

  // File austragen

  // The entry is destroyed here unless something else still shares it.
  de_ptr->files.erase(
    std::remove_if(
      std::begin(de_ptr->files),
      std::end(de_ptr->files),
      [&](const std::shared_ptr<FileEntry>& f)
      {
        return f.get() == this;
      }
    ),
    de_ptr->files.end()
  );

  return true;
}
