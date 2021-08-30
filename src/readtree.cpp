/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/readtree.c,v 1.13 2000/07/13 18:26:06 werner Exp $
 *
 * Funktionen zum Lesen des Dateibaumes
 *
 ***************************************************************************/


#include "ytree.h"



static void UnReadSubTree(DirEntry *dir_entry);



/* Dateibaum lesen: path = "Root"-Pfad
 * dir_entry wird von der Funktion gefuellt
 */


int ReadTree(DirEntry *dir_entry, const std::string& path, int depth)
{
  DIR           *dir;
  struct stat   stat_struct;
  DirEntry      first_dir_entry;
  DirEntry      *des_ptr;
  DirEntry      *den_ptr;
  FileEntry     first_file_entry;
  FileEntry     *fes_ptr;
  FileEntry     *fen_ptr;
  int		file_count;


  /* dir_entry initialisieren */
  /*--------------------------*/

  dir_entry->file           = NULL;
/*
  dir_entry->next           = NULL;
  dir_entry->prev           = NULL;
*/
  dir_entry->sub_tree       = NULL;
  dir_entry->total_bytes    = 0L;
  dir_entry->matching_bytes = 0L;
  dir_entry->tagged_bytes   = 0L;
  dir_entry->total_files    = 0;
  dir_entry->matching_files = 0;
  dir_entry->tagged_files   = 0;
  dir_entry->access_denied  = false;
  dir_entry->start_file     = 0;
  dir_entry->cursor_pos     = 0;
  dir_entry->global_flag    = false;
  dir_entry->login_flag     = false;
  dir_entry->big_window     = false;
  dir_entry->not_scanned    = false;

  if( S_ISBLK( dir_entry->stat_struct.st_mode ) )
    return( 0 ); /* Block-Device */

  if (depth < 0 && dir_entry->up_tree)
  {
    dir_entry->up_tree->not_scanned = true;

    return 1;
  }

  statistic.disk_total_directories++;

  if (!(dir = opendir(path.c_str())))
  {
    dir_entry->access_denied = true;

    return 1;
  }

  first_dir_entry.prev  = NULL;
  first_dir_entry.next  = NULL;
  *first_dir_entry.name = '\0';
  first_file_entry.next = NULL;
  fes_ptr               = &first_file_entry;

  file_count = 0;

  while (auto dirent = readdir(dir))
  {
    std::string new_path;

    if (!std::strcmp(dirent->d_name, ".") ||
        !std::strcmp(dirent->d_name, ".."))
    {
      continue;
    }

    if( EscapeKeyPressed() )
    {
      Quit();  /* Abfrage ob ytree verlassen werden soll */
    }

    if( ( file_count++ % 100 ) == 0 ) {
      DisplayDiskStatistic();
      doupdate();
    }

    new_path = path;
    if (new_path.compare(FILE_SEPARATOR_STRING))
    {
      new_path += FILE_SEPARATOR_CHAR;
    }
    new_path += dirent->d_name;

    if (STAT_(new_path.c_str(), &stat_struct))
    {
      if (errno != EACCES)
      {
        ErrorPrintf("stat() failed on*%s*IGNORED", new_path.c_str());
      }
      continue;
    }

    if( S_ISDIR( stat_struct.st_mode ) )
    {
      /* Directory-Entry */
      /*-----------------*/
      den_ptr = MallocOrAbort<DirEntry>(sizeof(DirEntry) + std::strlen(dirent->d_name));
      den_ptr->up_tree = dir_entry;

      std::strcpy( den_ptr->name, dirent->d_name );
      std::memcpy(
        static_cast<void*>(&den_ptr->stat_struct),
        static_cast<const void*>(&stat_struct),
        sizeof(stat_struct)
      );
      den_ptr->prev = nullptr;
      den_ptr->next = nullptr;

      ReadTree(den_ptr, new_path, depth - 1);

      /* Sortieren durch direktes Einfuegen */
      /*------------------------------------*/

      for( des_ptr = &first_dir_entry; des_ptr; des_ptr = des_ptr->next )
      {
        if( strcmp( des_ptr->name, den_ptr->name ) > 0 )
        {
	  /* des-Element ist groesser */
	  /*--------------------------*/

	  den_ptr->next = des_ptr;
	  den_ptr->prev = des_ptr->prev;
	  des_ptr->prev->next = den_ptr;
	  des_ptr->prev = den_ptr;
	  break;
	}

	if( des_ptr->next == NULL )
	{
	  /* Ende der Liste erreicht; ==> einfuegen */
	  /*----------------------------------------*/

          den_ptr->prev = des_ptr;
	  den_ptr->next = des_ptr->next;
          des_ptr->next = den_ptr;
	  break;
	}
      }
    }
    else
    {
      /* File-Entry */
      /*------------*/

      int n;
      char link_path[PATH_LENGTH + 1];

      /* Test, ob Eintrag Symbolischer Link ist */
      /*----------------------------------------*/

      n = 0; *link_path = '\0';

      if (S_ISLNK(stat_struct.st_mode))
      {
        /* Ja, symbolischer Name wird an "echten" Namen angehaengt */
        /*---------------------------------------------------------*/
        if ((n = readlink(new_path.c_str(), link_path, sizeof(link_path))) == -1)
        {
          std::strcpy(link_path, "unknown");
          n = std::strlen(link_path);
        }
        link_path[n] = 0;

        fen_ptr = MallocOrAbort<FileEntry>(sizeof(FileEntry) + std::strlen(dirent->d_name) + n + 1);
        std::strcpy(fen_ptr->name, dirent->d_name);
        std::strcpy(&fen_ptr->name[strlen(fen_ptr->name) + 1], link_path);
      } else {
        fen_ptr = MallocOrAbort<FileEntry>(sizeof(FileEntry) + std::strlen(dirent->d_name));
        std::strcpy(fen_ptr->name, dirent->d_name);
      }

      fen_ptr->next = nullptr;
      fen_ptr->prev = nullptr;
      fen_ptr->tagged = false;

      std::memcpy(
        static_cast<void*>(&fen_ptr->stat_struct),
        static_cast<const void*>(&stat_struct),
        sizeof(stat_struct)
      );

      fen_ptr->dir_entry = dir_entry;
      fes_ptr->next      = fen_ptr;
      fen_ptr->prev      = fes_ptr;
      fes_ptr            = fen_ptr;
      dir_entry->total_files++;
      dir_entry->total_bytes += stat_struct.st_size;
      statistic.disk_total_files++;
      statistic.disk_total_bytes += stat_struct.st_size;
    }
  }

  (void) closedir( dir );

  if( first_file_entry.next ) first_file_entry.next->prev = NULL;
  if( first_dir_entry.next )  first_dir_entry.next->prev = NULL;

  dir_entry->file = first_file_entry.next;
  dir_entry->sub_tree = first_dir_entry.next;

  DisplayDiskStatistic();
  doupdate();

  return( 0 );
}



void UnReadTree(DirEntry *dir_entry)
{
  FileEntry *fe_ptr, *next_fe_ptr;

  if( dir_entry == statistic.tree )
  {
    MESSAGE( "Can't delete ROOT" );
  }
  else
  {
    for( fe_ptr=dir_entry->file; fe_ptr; fe_ptr=next_fe_ptr )
    {
      next_fe_ptr = fe_ptr->next;
      RemoveFile( fe_ptr );
    }
    if( dir_entry->sub_tree )
    {
      UnReadSubTree( dir_entry->sub_tree );
    }
    statistic.disk_total_directories--;
    (void) GetAvailBytes( &statistic.disk_space );
    DisplayDiskStatistic();
    doupdate();
  }
}


static void UnReadSubTree(DirEntry *dir_entry)
{
  DirEntry *de_ptr, *next_de_ptr;
  FileEntry *fe_ptr, *next_fe_ptr;

  for( de_ptr = dir_entry; de_ptr; de_ptr = next_de_ptr )
  {
    next_de_ptr = de_ptr->next;

    for( fe_ptr=de_ptr->file; fe_ptr; fe_ptr=next_fe_ptr )
    {
      next_fe_ptr = fe_ptr->next;
      RemoveFile( fe_ptr );
    }

    if( de_ptr->sub_tree )
    {
      UnReadSubTree( de_ptr->sub_tree );
    }

    if( !de_ptr->up_tree->not_scanned )
      statistic.disk_total_directories--;

    if( de_ptr->prev ) de_ptr->prev->next = de_ptr->next;
    else de_ptr->up_tree->sub_tree = de_ptr->next;
    if( de_ptr->next ) de_ptr->next->prev = de_ptr->prev;

    free( de_ptr );
  }
}

