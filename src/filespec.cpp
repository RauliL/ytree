/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/filespec.c,v 1.13 2005/01/22 16:32:29 werner Exp $
 *
 * Setzt neue Datei-Spezifikation
 *
 ***************************************************************************/


#include "ytree.h"




int SetFileSpec(char *file_spec)
{
  if( SetMatchSpec( file_spec ) )
  {
    return( 1 );
  }

  statistic.disk_matching_files = 0L;
  statistic.disk_matching_bytes = 0L;

  SetMatchingParam( statistic.tree );

  return( 0 );
}




void SetMatchingParam(DirEntry *dir_entry)
{
  unsigned long matching_files = 0L;
  long long matching_bytes = 0L;

  for( FileEntry *fe_ptr : dir_entry->files )
  {
    if( Match( fe_ptr->name ) )
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

  for( DirEntry *child : dir_entry->children )
  {
    SetMatchingParam( child );
  }
}



/***************************************************************>>
ReadFileSpec.
Take in the user-specified new filespec.
As modified, it defaults to '*'; the original version offered the
current value as default, but that's just an up-arrow away.
Returns 0 on success, -1 on failure (empty string).
<<***************************************************************/

int ReadFileSpec(void)
{
  int result = -1;

  char buffer[FILE_SPEC_LENGTH * 2 + 1];

  ClearHelp();

  *std::format_to(buffer, "{}", "*") = '\0';
  MvAddStr( LINES - 2, 1, "New filespec:" );
  if (InputString( buffer, LINES - 2, 15, 0, FILE_SPEC_LENGTH) == CR)
  {
    if( SetFileSpec( buffer ) )
    {
      Message("Invalid Filespec");
    }
    else
    {
      *std::format_to(statistic.file_spec, "{}", buffer) = '\0';
      result = 0;
    }
  }
  move( LINES - 2, 1 ); clrtoeol();
  return(result);
}


