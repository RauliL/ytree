/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/rmdir.c,v 1.11 2001/06/15 16:36:36 werner Exp $
 *
 * Loeschen von Verzeichnissen
 *
 ***************************************************************************/


#include "ytree.h"



static int DeleteSubTree(DirEntry *dir_entry);
static int DeleteSingleDirectory(DirEntry *dir_entry);


int DeleteDirectory(DirEntry *dir_entry)
{
  int result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  ClearHelp();

  if (dir_entry == statistic.tree)
  {
    Message("Can't delete ROOT");
  }
  else if( dir_entry->file || dir_entry->sub_tree )
  {
    if( InputChoise( "Directory not empty, PRUNE ? (Y/N) ? ", "YN\033" ) == 'Y' ) {
      if( dir_entry->sub_tree ) {
        ScanSubTree(dir_entry);
        if( DeleteSubTree( dir_entry->sub_tree ) ) {
          ESCAPE;
        }
      }
      if( DeleteSingleDirectory( dir_entry ) ) {
	ESCAPE;
      }
      result = 0;
      ESCAPE;
    }
  }
  else if( InputChoise( "Delete this directory (Y/N) ? ", "YN\033" ) == 'Y' )
  {
    const auto path = GetPath(dir_entry);

    if (!IsWriteable(path))
    {
      MessagePrintf(
        "Can't delete directory*\"%s\"*%s",
		    path.c_str(),
        std::strerror(errno)
      );
    }
    else if (rmdir(path.c_str()))
    {
      MessagePrintf(
        "Can't delete directory*\"%s\"*%s",
		    path.c_str(),
        std::strerror(errno)
      );
    } else {
      /* Directory geloescht
       * ==> aus Baum loeschen
       */

      statistic.disk_total_directories--;

      if( dir_entry->prev ) dir_entry->prev->next = dir_entry->next;
      else dir_entry->up_tree->sub_tree = dir_entry->next;

      if( dir_entry->next ) dir_entry->next->prev = dir_entry->prev;

      std::free(static_cast<void*>(dir_entry));

      (void) GetAvailBytes( &statistic.disk_space );

      result = 0;
    }
  }

FNC_XIT:

  return( result );
}





static int DeleteSubTree( DirEntry *dir_entry )
{
  int result = -1;
  DirEntry *de_ptr, *next_de_ptr;


  for( de_ptr = dir_entry; de_ptr; de_ptr = next_de_ptr ) {
    next_de_ptr = de_ptr->next;

    if( de_ptr->sub_tree ) {
      if( DeleteSubTree( de_ptr->sub_tree ) ) {
        ESCAPE;
      }
    }
    if( DeleteSingleDirectory( de_ptr ) ) {
      ESCAPE;
    }
  }

  result = 0;

FNC_XIT:

    return( result );
}



static int DeleteSingleDirectory( DirEntry *dir_entry )
{
  const auto path = GetPath(dir_entry);
  FileEntry* next_fe_ptr = nullptr;

  if (!IsWriteable(path))
  {
    MessagePrintf(
      "Can't delete directory*\"%s\"*%s",
		  path.c_str(),
      std::strerror(errno)
		);

    return -1;
  }

  for (auto fe_ptr = dir_entry->file; fe_ptr; fe_ptr = next_fe_ptr)
  {
    next_fe_ptr = fe_ptr->next;
    if (DeleteFile(fe_ptr))
    {
      return -1;
    }
  }

  if (rmdir(path.c_str()))
  {
    MessagePrintf(
      "Can't delete directory*\"%s\"*%s",
		  path.c_str(),
      std::strerror(errno)
    );

    return -1;
  }

  if( !dir_entry->up_tree->not_scanned )
    statistic.disk_total_directories--;

  if( dir_entry->prev ) dir_entry->prev->next = dir_entry->next;
  else dir_entry->up_tree->sub_tree = dir_entry->next;
  if( dir_entry->next ) dir_entry->next->prev = dir_entry->prev;

  std::free(static_cast<void*>(dir_entry));

  return 0;
}
