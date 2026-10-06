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
static void UnlinkDirEntry(DirEntry *dir_entry);


int DeleteDirectory(DirEntry *dir_entry)
{
  int result = -1;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  ClearHelp();

  if (dir_entry == statistic.tree)
  {
    Message("Can't delete ROOT");
  }
  else if( !dir_entry->files.empty() || !dir_entry->children.empty() )
  {
    if( InputChoise( "Directory not empty, PRUNE ? (Y/N) ? ", "YN\033" ) == 'Y' ) {
      if( !dir_entry->children.empty() ) {
        ScanSubTree(dir_entry);
        if( DeleteSubTree( dir_entry ) ) {
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
      FormatMessage("Can't delete directory*\"{}\"*{}", path.c_str(), std::strerror(errno));
    }
    else if (rmdir(path.c_str()))
    {
      FormatMessage("Can't delete directory*\"{}\"*{}", path.c_str(), std::strerror(errno));
    } else {
      /* Directory geloescht
       * ==> aus Baum loeschen
       */

      statistic.disk_total_directories--;

      UnlinkDirEntry( dir_entry );

      delete dir_entry;

      (void) GetAvailBytes( &statistic.disk_space );

      result = 0;
    }
  }

FNC_XIT:

  return( result );
}





/* Loescht alle Unterverzeichnisse von dir_entry */
/*-----------------------------------------------*/

static int DeleteSubTree( DirEntry *dir_entry )
{
  int result = -1;

  /* Kopie, da DeleteSingleDirectory() die Eintraege austraegt */
  const auto children = dir_entry->children;

  for( DirEntry *de_ptr : children ) {
    if( !de_ptr->children.empty() ) {
      if( DeleteSubTree( de_ptr ) ) {
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

  if (!IsWriteable(path))
  {
    FormatMessage("Can't delete directory*\"{}\"*{}", path.c_str(), std::strerror(errno));

    return -1;
  }

  /* Kopie, da DeleteFile() die Eintraege austraegt */
  const auto files = dir_entry->files;

  for (const auto fe_ptr : files)
  {
    if (DeleteFile(fe_ptr))
    {
      return -1;
    }
  }

  if (rmdir(path.c_str()))
  {
    FormatMessage("Can't delete directory*\"{}\"*{}", path.c_str(), std::strerror(errno));

    return -1;
  }

  if( !dir_entry->parent->not_scanned )
    statistic.disk_total_directories--;

  UnlinkDirEntry( dir_entry );

  delete dir_entry;

  return 0;
}


/* Traegt dir_entry aus der Liste der Unterverzeichnisse seines Vaters aus */
/*-------------------------------------------------------------------------*/

static void UnlinkDirEntry( DirEntry *dir_entry )
{
  auto& siblings = dir_entry->parent->children;

  siblings.erase(
    std::remove( siblings.begin(), siblings.end(), dir_entry ),
    siblings.end()
  );
}
