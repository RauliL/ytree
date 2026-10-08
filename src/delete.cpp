/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/delete.c,v 1.12 2001/06/15 16:36:36 werner Exp $
 *
 * Loeschen von Dateien / Verzeichnissen
 *
 ***************************************************************************/
#include "ytree.h"

int DeleteFile(FileEntry *fe_ptr)
{
  const auto filepath = GetFileNamePath(fe_ptr);
  char     buffer[PATH_LENGTH+1];
  int      result;
  int      term;
  std::error_code ec;

  result = -1;

  if (!S_ISLNK(fe_ptr->stat_struct.st_mode))
  {
    if (!IsWriteable(filepath))
    {
      if (!std::filesystem::exists(filepath))
      {
        /* Datei existiert nicht ==> fertig */
        goto UNLINK_DONE;
      }

      std::snprintf(
        buffer,
        sizeof(buffer),
        "overriding mode %04o for \"%s\" (Y/N) ? ",
        fe_ptr->stat_struct.st_mode & 0777,
        fe_ptr->name.c_str()
);

      term = InputChoise(buffer, "YN\033");

      if( term != 'Y' )
      {
        FormatMessage("Can't delete file*\"{}\"*{}", filepath.c_str(), std::strerror(errno));
        ESCAPE;
      }
    }
  }

  std::filesystem::remove(filepath, ec);
  if (ec)
  {
    FormatMessage("Can't delete file*\"{}\"*{}", filepath.c_str(), ec.message().c_str());
    ESCAPE;
  }

UNLINK_DONE:

  /* File austragen */
  /*----------------*/

  result = RemoveFile(fe_ptr);
  GetAvailBytes(&statistic.disk_space);

FNC_XIT:

  return( result );
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







