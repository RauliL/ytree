#include "ytree.h"

int DeleteFile(FileEntry* fe_ptr)
{
  const auto filepath = fe_ptr->GetPath();
  int result = -1;
  std::error_code ec;

  if (!S_ISLNK(fe_ptr->stat_struct.st_mode))
  {
    if (!IsWriteable(filepath))
    {
      int term;

      if (!std::filesystem::exists(filepath))
      {
        // Datei existiert nicht ==> fertig
        goto UNLINK_DONE;
      }

      term = InputChoise(
        std::format(
          "overriding mode {:04o} for \"{}\" (Y/N) ? ",
          fe_ptr->stat_struct.st_mode & 0777,
          fe_ptr->name
        ),
        "YN\033"
      );

      if (term != 'Y')
      {
        FormatMessage(
          "Can't delete file*\"{}\"*{}",
          filepath.string(),
          std::strerror(errno)
        );

        return result;
      }
    }
  }

  std::filesystem::remove(filepath, ec);
  if (ec)
  {
    FormatMessage(
      "Can't delete file*\"{}\"*{}",
      filepath.string(),
      ec.message()
    );

    return result;
  }

UNLINK_DONE:
  // File austragen
  result = RemoveFile(fe_ptr);
  GetAvailBytes(&statistic.disk_space);

  return result;
}

int RemoveFile(FileEntry *fe_ptr)
{
  std::int64_t file_size;

  const auto de_ptr = fe_ptr->Dir();

  if (!de_ptr)
  {
    return -1;
  }

  file_size = fe_ptr->stat_struct.st_size;

  de_ptr->total_bytes -= file_size;
  de_ptr->total_files--;
  statistic.disk_total_bytes -= file_size;
  statistic.disk_total_files--;
  if( fe_ptr->matching ) {
    de_ptr->matching_bytes -= file_size;
    de_ptr->matching_files--;
    statistic.disk_matching_bytes -= file_size;
    statistic.disk_matching_files--;
  }
  if( fe_ptr->tagged )
  {
    de_ptr->tagged_bytes -= file_size;
    de_ptr->tagged_files--;
    statistic.disk_tagged_bytes -= file_size;
    statistic.disk_tagged_files--;
  }

  /* File austragen */
  /*----------------*/

  /* The entry is destroyed here unless something else still shares it. */
  de_ptr->files.erase(
    std::remove_if(
      de_ptr->files.begin(),
      de_ptr->files.end(),
      [fe_ptr](const std::shared_ptr<FileEntry>& f) { return f.get() == fe_ptr; }
),
    de_ptr->files.end()
);

  return( 0 );
}







