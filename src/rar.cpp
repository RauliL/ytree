/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/rar.c,v 1.3 2000/06/10 13:15:41 werner Exp $
 *
 * Funktionen zum Lesen des Dateibaumes aus RAR-Dateien
 *
 ***************************************************************************/


#include "ytree.h"


static int GetStatFromRAR(char *rar_line, char *name, struct stat *stat);



/* Dateibaum aus RAR-Listing lesen */
/*---------------------------------*/

int ReadTreeFromRAR(const std::shared_ptr<DirEntry>& dir_entry, FILE *f)
{
  char rar_line[RAR_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;
  bool dir_flag = false;

  dir_entry->name.clear();

  while( std::fgets(rar_line, RAR_LINE_LENGTH, f) != nullptr )
  {
    /* \n loeschen */
    /*-------------*/

    rar_line[ std::strlen(rar_line) - 1 ] = '\0';

    if( std::strlen(rar_line) > (unsigned) 48 && rar_line[48] == ':' )
    {
      /* gueltiger Eintrag */
      /*-------------------*/

      if( GetStatFromRAR(rar_line, path_name, &stat) )
      {
        FormatMessage("unknown rarinfo*{}", rar_line);
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






static int GetStatFromRAR(char *rar_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  id;
  struct tm tm_struct;


  std::memset(stat, 0, sizeof( struct stat ));

  stat->st_nlink = 1;

  t = Strtok_r(rar_line, " \t", &old); if( t == nullptr ) return( -1 );

  /* Dateiname */
  /*-----------*/

  std::strcpy(name, t);
  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* Dateilaenge */
  /*-------------*/

  if( !std::isdigit(*t) ) return( -1 );
  stat->st_size = AtoLL(t);
  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* Packed */
  /*--------*/

  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* Ratio */
  /*-------*/

  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* M-Datum */
  /*---------*/

  if(std::strlen(t) == 8) {
    t[2] = t[5] = '\0';
    tm_struct.tm_mday = std::atoi(&t[0]);
    tm_struct.tm_mon  = std::atoi(&t[3]);
    tm_struct.tm_year = std::atoi(&t[6]);

    if(tm_struct.tm_year < 70)
       tm_struct.tm_year += 100;
  }

  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* M-time */
  /*--------*/

  if(std::strlen(t) == 5) {
    t[2] = '\0';
    tm_struct.tm_hour = std::atoi(&t[0]);
    tm_struct.tm_min  = std::atoi(&t[2]);
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = Mktime(&tm_struct);

  t = Strtok_r(nullptr, " \t", &old); if( t == nullptr ) return( -1 );

  /* Attributes */
  /*------------*/

  stat->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  /* Owner */
  /*-------*/

  id = getuid();
  if( id == -1 ) id = std::atoi(t);
  stat->st_uid = (unsigned) id;

  /* Group */
  /*-------*/

  id = getgid();
  stat->st_gid = (unsigned) id;

  return( 0 );
}


