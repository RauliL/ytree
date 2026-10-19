#include "./ytree.hpp"

bool SetFileSpec(const std::string& file_spec)
{
  if (!SetMatchSpec(file_spec))
  {
    return false;
  }

  statistic.disk_matching_files = 0L;
  statistic.disk_matching_bytes = 0L;

  statistic.tree->SetMatchingParam();

  return true;
}

void DirEntry::SetMatchingParam()
{
  unsigned long new_matching_files = 0L;
  std::int64_t new_matching_bytes = 0L;

  for (const auto& fe_ptr : files)
  {
    if (Match(fe_ptr->name))
    {
      ++new_matching_files;
      new_matching_bytes += fe_ptr->meta.size;
      fe_ptr->matching = true;
    } else {
      fe_ptr->matching = false;
    }
  }

  matching_files = new_matching_files;
  matching_bytes = new_matching_bytes;

  statistic.disk_matching_files += new_matching_files;
  statistic.disk_matching_bytes += new_matching_bytes;

  for (const auto& child : children)
  {
    child->SetMatchingParam();
  }
}

/***************************************************************>>
ReadFileSpec.
Take in the user-specified new filespec.
As modified, it defaults to '*'; the original version offered the
current value as default, but that's just an up-arrow away.
Returns the new filespec on success, or nullopt on cancel/invalid input.
<<***************************************************************/

std::optional<std::string> ReadFileSpec()
{
  std::string buffer = "*";

  ClearHelp();

  MvAddStr(LINES - 2, 1, "New filespec:");
  const auto filespec = InputString(buffer, LINES - 2, 15, 0, FILE_SPEC_LENGTH);
  move(LINES - 2, 1);
  clrtoeol();

  if (!filespec)
  {
    return std::nullopt;
  }

  if (!SetFileSpec(*filespec))
  {
    Message("Invalid Filespec");
    return std::nullopt;
  }

  statistic.file_spec = *filespec;
  return filespec;
}
