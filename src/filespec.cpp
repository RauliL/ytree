/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/filespec.c,v 1.13 2005/01/22 16:32:29 werner Exp $
 *
 * Setzt neue Datei-Spezifikation
 *
 ***************************************************************************/


#include "ytree.h"




bool SetFileSpec(const std::string& file_spec)
{
  if (SetMatchSpec(file_spec))
  {
    return false;
  }

  statistic.disk_matching_files = 0L;
  statistic.disk_matching_bytes = 0L;

  SetMatchingParam(statistic.tree.get());

  return true;
}




void SetMatchingParam(DirEntry *dir_entry)
{
  unsigned long matching_files = 0L;
  std::int64_t matching_bytes = 0L;

  for( const auto& fe_ptr : dir_entry->files )
  {
    if( Match(fe_ptr->name) )
    {
      matching_files++;
      matching_bytes += fe_ptr->stat_struct.st_size;
      fe_ptr->matching = true;
    }
    else
    {
      fe_ptr->matching = false;
    }
  }

  dir_entry->matching_files = matching_files;
  dir_entry->matching_bytes = matching_bytes;

  statistic.disk_matching_files += matching_files;
  statistic.disk_matching_bytes += matching_bytes;

  for( const auto& child : dir_entry->children )
  {
    SetMatchingParam(child.get());
  }
}



/***************************************************************>>
ReadFileSpec.
Take in the user-specified new filespec.
As modified, it defaults to '*'; the original version offered the
current value as default, but that's just an up-arrow away.
Returns 0 on success, -1 on failure (empty string).
<<***************************************************************/

int ReadFileSpec()
{
  int result = -1;

  std::string buffer = "*";

  ClearHelp();

  MvAddStr(LINES - 2, 1, "New filespec:");
  if (InputString(buffer, LINES - 2, 15, 0, FILE_SPEC_LENGTH) == CR)
  {
    if (!SetFileSpec(buffer))
    {
      Message("Invalid Filespec");
    }
    else
    {
      statistic.file_spec = buffer;
      result = 0;
    }
  }
  move(LINES - 2, 1); clrtoeol();
  return(result);
}


