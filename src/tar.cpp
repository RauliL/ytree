/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/tar.c,v 1.11 2000/05/20 20:41:11 werner Exp $
 *
 * Funktionen zum Lesen des Dateibaumes aus TAR-Dateien
 *
 ***************************************************************************/


#include "ytree.h"



static int GetStatFromTAR(char *tar_line, char *name, struct stat *stat);



/* Dateibaum aus TAR-Listing lesen */
/*---------------------------------*/

int ReadTreeFromTAR(const std::shared_ptr<DirEntry>& dir_entry, FILE *f)
{
  char tar_line[TAR_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;
  bool   dir_flag = false;

  dir_entry->name.clear();

  while( std::fgets(tar_line, TAR_LINE_LENGTH, f) != nullptr )
  {
    /* \n loeschen */
    /*-------------*/

    tar_line[ std::strlen(tar_line) - 1 ] = '\0';

    if( GetStatFromTAR(tar_line, path_name, &stat) )
    {
      FormatMessage("unknown tarinfo*{}", tar_line);
    }
    else
    {
      if (
        (path_name[std::strlen(path_name) - 1] == std::filesystem::path::preferred_separator) ||
	    !std::strcmp(path_name, ".") ||
	    *tar_line == 'd'
	  )
      {
        /* Directory */
        /*-----------*/

#ifdef DEBUG
  std::fprintf(stderr, "DIR: %s\n", path_name);
#endif

	if( std::strcmp(path_name, "./") )
	{
	  /* "./" wird ignoriert */
	  /*---------------------*/

          TryInsertArchiveDirEntry(dir_entry, path_name, &stat);
	  DisplayDiskStatistic();
	  doupdate();
	}
      }
      else
      {
        /* File */
        /*------*/

#ifdef DEBUG
  std::fprintf(stderr, "FILE: \"%s\"\n", path_name);
#endif
        InsertArchiveFileEntry(dir_entry, path_name, &stat);
      }
    }
  }

  if( dir_flag == false )
  {
    statistic.disk_total_directories++;
    std::memset((char *) &dir_entry->stat_struct, 0, sizeof( struct stat ));
    dir_entry->stat_struct.st_mode = S_IFDIR;
  }
  return( MinimizeArchiveTree(dir_entry) );
}





static int GetStatFromTAR(char *tar_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  i, id;
  struct tm tm_struct;
  static const char *month[] = { "Jan", "Feb", "Mar", "Apr", "Mai", "Jun",
	 	           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


  std::memset(stat, 0, sizeof( struct stat ));

  stat->st_nlink = 1;

  t = Strtok_r(tar_line, " \t", &old); if( t == nullptr ) return( -1 );

  /* Attribute */
  /*-----------*/

  if( std::strlen(t) != 10 ) return( -1 );
  stat->st_mode = GetModus(t);
  t = Strtok_r(nullptr, " \t/", &old); if( t == nullptr ) return( -1 );


  /* Owner */
  /*-------*/
  if (const auto owner_id_ptr = GetPasswdUid(t))
  {
    id = *owner_id_ptr;
  } else {
    id = std::atoi(t);
  }
  stat->st_uid = (unsigned) id;

  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* Group */
  /*-------*/
  if (const auto group_id_ptr = GetGroupId(t))
  {
    id = *group_id_ptr;
  } else {
    id = std::atoi(t);
  }
  stat->st_gid = (unsigned) id;

  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* Dateilaenge */
  /*-------------*/

  if( !std::isdigit(*t) ) return( -1 );
  stat->st_size = AtoLL(t);
  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* M-Datum */
  /*---------*/

  for( i=0; i < 12; i++ )
  {
    if( !std::strcmp(t, month[i]) ) break;
  }
  if( i >= 12 )
  {
    t[4] = t[7] = '\0';

    tm_struct.tm_year = std::atoi(t) - 1900;
    tm_struct.tm_mon  = std::atoi(&t[5]) - 1;
    tm_struct.tm_mday = std::atoi(&t[8]);

    t = Strtok_r(nullptr, " \t:", &old); if( t == nullptr ) return( -1 );
    tm_struct.tm_hour = std::atoi(t);
    t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );
    tm_struct.tm_min = std::atoi(t);
    t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );
    goto XDATE;
  }


  tm_struct.tm_mon = i;
  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  tm_struct.tm_mday = std::atoi(t);
  t = Strtok_r(nullptr, " \t:", &old); if( t == nullptr ) return( -1 );

  tm_struct.tm_hour = std::atoi(t);
  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  tm_struct.tm_min = std::atoi(t);
  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  tm_struct.tm_year = std::atoi(t) - 1900;
  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

XDATE:

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = Mktime(&tm_struct);

  /* Dateiname */
  /*-----------*/

  std::strcpy(name, t);


  if( S_ISLNK(stat->st_mode) )
  {
    /* Symbolischer Link */
    /*-------------------*/

    t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );
    t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );
    std::strcpy(&name[ std::strlen(name) + 1 ], t);
  }

  return( 0 );
}


