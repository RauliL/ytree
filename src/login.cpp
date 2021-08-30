/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/login.c,v 1.23 2014/12/26 09:53:11 werner Exp $
 *
 * Dateibaum lesen
 *
 ***************************************************************************/


#include "ytree.h"
/* #include <sys/wait.h> */  /* maybe wait.h is available */




static void DeleteTree(DirEntry *tree)
{
  DirEntry  *de_ptr, *next_de_ptr;
  FileEntry *fe_ptr, *next_fe_ptr;

  for( de_ptr=tree; de_ptr; de_ptr=next_de_ptr)
  {
    next_de_ptr = de_ptr->next;

    for( fe_ptr=de_ptr->file; fe_ptr; fe_ptr=next_fe_ptr)
    {
      next_fe_ptr=fe_ptr->next;
      free( fe_ptr );
    }

    if( de_ptr->sub_tree ) DeleteTree( de_ptr->sub_tree );

    free( de_ptr );
  }
}




/* Login Disk liefert
 * -1 bei Fehler
 * 0  bei fehlerfreiem lesen eines neuen Baumes
 * 1  bei Benutzung des Baumes im Speicher
 */


int LoginDisk(char *path)
{
  struct stat stat_struct;
  char   command_line[COMMAND_LINE_LENGTH + 1];
  char   cat_file_name[PATH_LENGTH+1];
  std::optional<CompressMethod> file_method;
  int    pid;
  int    p[2];
  int    depth, l = 0;
  FILE   *f;
  int    status;
  int    result = 0;

  if( mode == DISK_MODE || mode == USER_MODE)
  {
    /* Status retten */
    /*---------------*/
    (void) memcpy( (char *) &disk_statistic,
		   (char *) &statistic,
		   sizeof( Statistic )
		 );
  }

  if ( disk_statistic.login_path[0] != 0) {
    if( !strcmp( disk_statistic.login_path, path ) )
    {
      /* Tree is in memory! Use it! */
      /*----------------------------*/

      if( statistic.tree != disk_statistic.tree )
        DeleteTree( statistic.tree );

      if (IsUserActionDefined())
      {
        mode = USER_MODE;
      }
      else
      {
        mode = DISK_MODE;
      }
      (void) memcpy( (char *) &statistic,
                     (char *) &disk_statistic,
                     sizeof( Statistic )
                     );
      (void) SetFileSpec( statistic.file_spec );
      return( 1 );   /* Return-Wert fuer "alten Baum" */
    }
  }


  if( STAT_( path, &stat_struct ) )
  {
    /* Stat failed */
    /*-------------*/
    MessagePrintf("Can't access*\"%s\"*%s", path, std::strerror(errno));

    return -1;
  }


  if( mode != DISK_MODE && mode != USER_MODE )
  {
    DeleteTree( statistic.tree );
  }

  (void) memset( &statistic, 0, sizeof( statistic ) );

  statistic.tree = MallocOrAbort<DirEntry>(sizeof(DirEntry) + PATH_LENGTH);

  (void) memset( statistic.tree, 0, sizeof( DirEntry ) + PATH_LENGTH );

  (void) strcpy( statistic.path, path );
  (void) strcpy( statistic.login_path, path );
  (void) strcpy( statistic.file_spec, DEFAULT_FILE_SPEC );
  (void) strcpy( statistic.tape_name, DEFAULT_TAPEDEV );
  statistic.kind_of_sort = SORT_BY_NAME + SORT_ASC;
  (void) memcpy( &statistic.tree->stat_struct,
		 &stat_struct,
		 sizeof( stat_struct )
	       );


  if( !S_ISDIR(stat_struct.st_mode ) )
  {
    /* No Directory ==> TAR_FILE/RPM/ZOO/ZIP/LHA/ARC_FILE */
    /*----------------------------------------------------*/
    file_method = GetFileMethod(statistic.login_path);
    l = std::strlen(statistic.login_path);
    if (!file_method)
    {
      mode = TAR_FILE_MODE;
    } else {
      switch (*file_method)
      {
        case CompressMethod::ZOO_COMPRESS:
          mode = ZOO_FILE_MODE;
          break;

        case CompressMethod::ARC_COMPRESS:
          mode = ARC_FILE_MODE;
          break;

        case CompressMethod::LHA_COMPRESS:
          mode = LHA_FILE_MODE;
          break;

        case CompressMethod::ZIP_COMPRESS:
          mode = ZIP_FILE_MODE;
          break;

        case CompressMethod::RPM_COMPRESS:
          mode = RPM_FILE_MODE;
          break;

        case CompressMethod::RAR_COMPRESS:
          mode = RAR_FILE_MODE;
          break;

        case CompressMethod::TAPE_DIR_NO_COMPRESS:
        case CompressMethod::TAPE_DIR_COMPRESS_COMPRESS:
        case CompressMethod::TAPE_DIR_FREEZE_COMPRESS:
        case CompressMethod::TAPE_DIR_GZIP_COMPRESS:
        case CompressMethod::TAPE_DIR_BZIP_COMPRESS:
          mode = TAPE_MODE;
          break;

        default:
          mode = TAR_FILE_MODE;
          break;
      }
    }
  }
  else if (IsUserActionDefined())
  {
    mode = USER_MODE;
  }
  else
  {
    mode = DISK_MODE;
  }


  (void) GetDiskParameter( path,
			   statistic.disk_name,
			   &statistic.disk_space,
			   &statistic.disk_capacity
			 );

  RefreshWindow( stdscr );
  RefreshWindow( dir_window );
  DisplayMenu();
  doupdate();


  if( mode == TAPE_MODE )
  {
    /* zugehoeriges tape-device ermitteln */
    /*------------------------------------*/

    if( GetTapeDeviceName() )
    {
      return( -1 );
    }
  }


  if( mode != DISK_MODE && mode != USER_MODE)
  {
    (void) strcpy( statistic.tree->name, path );

    if (pipe(p))
    {
      Error("pipe() failed");

      return -1;
    }


    if (!file_method)
    {
      /* NO_COMPRESS */
      /*-------------*/

      /* gtar tvf - < TAR_FILE */
      /*-----------------------*/

      (void) sprintf( command_line, "%s < '%s'",
		      TARLIST,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::ZOO_COMPRESS)
    {
      /* zoo vom ZOO_FILE */
      /*------------------*/

      (void) sprintf( command_line, "%s '%s'",
                      ZOOLIST,
                      statistic.login_path
                    );
    }
    else if (*file_method == CompressMethod::RPM_COMPRESS)
    {
      /* rpm vom RPM_FILE */
      /*------------------*/

      (void) sprintf( command_line, "%s '%s'",
                      RPMLIST,
                      statistic.login_path
                    );
    }
    else if (*file_method == CompressMethod::LHA_COMPRESS)
    {
      /* LHA_FILE */
      /*----------*/

      (void) sprintf( command_line, "%s '%s'",
		      LHALIST,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::ZIP_COMPRESS)
    {
      /* ZIP_FILE */
      /*----------*/

      (void) sprintf( command_line, "%s '%s'",
		      ZIPLIST,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::ARC_COMPRESS)
    {
      /* ARC_FILE */
      /*----------*/

      (void) sprintf( command_line, "%s '%s'",
		      ARCLIST,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::RAR_COMPRESS)
    {
      /* RAR_FILE */
      /*----------*/

      (void) sprintf( command_line, "%s '%s'",
		      RARLIST,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::FREEZE_COMPRESS)
    {
      /* melt < TAR_FILE | gtar tvf - */
      /*------------------------------*/

      (void) sprintf( command_line, "%s < '%s' %s | %s",
		      MELT,
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if (*file_method == CompressMethod::MULTIPLE_FREEZE_COMPRESS)
    {
      (void) strncpy( cat_file_name, statistic.login_path, l - 2 );
      (void) strcpy( &cat_file_name[l - 2], "*" );

      /* cat TAR_FILE | melt | gtar tvf - */
      /*----------------------------------*/

      (void) sprintf( command_line, "%s '%s' %s | %s | %s",
		      CAT,
		      cat_file_name,
		      ERR_TO_STDOUT,
		      MELT,
		      TARLIST
		    );
    }
    else if (*file_method == CompressMethod::COMPRESS_COMPRESS)
    {
      /* uncompress < TAR_FILE | gtar tvf - */
      /*------------------------------------*/

      (void) sprintf( command_line, "%s < '%s' %s | %s",
		      UNCOMPRESS,
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if (*file_method == CompressMethod::MULTIPLE_COMPRESS_COMPRESS)
    {
      (void) strncpy( cat_file_name, statistic.login_path, l - 2 );
      (void) strcpy( &cat_file_name[l - 2], "*" );

      /* cat TAR_FILE.X* | uncompress | gtar tvf - */
      /*-------------------------------------------*/

      (void) sprintf( command_line, "%s %s | %s %s | %s",
		      CAT,
		      cat_file_name,
		      UNCOMPRESS,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if (*file_method == CompressMethod::GZIP_COMPRESS)
    {
      /* gunzip < TAR_FILE | gtar tvf - */
      /*--------------------------------*/

      (void) sprintf( command_line, "%s < '%s' %s | %s",
		      GNUUNZIP,
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if (*file_method == CompressMethod::MULTIPLE_GZIP_COMPRESS)
    {
      (void) strncpy( cat_file_name, statistic.login_path, l - 2 );
      (void) strcpy( &cat_file_name[l - 2], "*" );

      /* cat TAR_FILE.X* | gunzip | gtar tvf - */
      /*---------------------------------------*/

      (void) sprintf( command_line, "%s %s | %s %s | %s",
		      CAT,
		      cat_file_name,
		      GNUUNZIP,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if (*file_method == CompressMethod::BZIP_COMPRESS)
    {
      /* bunzip2 < TAR_FILE | gtar tvf - */
      /*---------------------------------*/

      (void) sprintf( command_line, "%s < '%s' %s | %s",
		      BUNZIP,
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_FREEZE_COMPRESS)
    {
      /* melt < TAR_FILE */
      /*-----------------*/

      (void) sprintf( command_line, "%s < '%s'",
		      MELT,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_COMPRESS_COMPRESS)
    {
      /* uncompress < TAR_FILE */
      /*-----------------------*/

      (void) sprintf( command_line, "%s < '%s'",
		      UNCOMPRESS,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_GZIP_COMPRESS)
    {
      /* gunzip < TAR_FILE */
      /*-------------------*/

      (void) sprintf( command_line, "%s < '%s'",
		      GNUUNZIP,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_BZIP_COMPRESS)
    {
      /* bunzip2 < TAR_FILE */
      /*--------------------*/

      (void) sprintf( command_line, "%s < '%s'",
		      BUNZIP,
		      statistic.login_path
		    );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_NO_COMPRESS)
    {
      /* cat < TAR_FILE */
      /*----------------*/

      (void) sprintf( command_line, "%s < '%s'",
		      CAT,
		      statistic.login_path
		    );
    }
    else
    {
      ErrorPrintf(
        "unknown file method %d",
        static_cast<int>(*file_method)
      );
      *command_line = 0;
      close(p[0]);
      close(p[1]);

      return -1;
    }

    (void) strcat( command_line, ERR_TO_NULL );

#ifdef DEBUG
  fprintf( stderr, "system( \"%s\" )\n", command_line );
#endif

    pid = fork();

    if (pid == -1)
    {
      Error("fork() failed");
      close(p[0]);
      close(p[1]);

      return -1;
    }
    else if( pid == 0 )
    {
      /* Sohn */
      /*------*/

      (void) close( p[0] );
      (void) close( 1 );
      if(dup( p[1] ) == -1)
      {
	/* (void) fprintf(stderr, "dup failed\n" ); */
      }
      (void) close( p[1] );

      if( result == 0 && SilentSystemCallEx( command_line, false ) )
      {
        result = 1;
	/* (void) fprintf(stderr, "system(%s)*failed\n", command_line ); */
      }
      exit( result );
    }
    else
    {
      /* Vater */
      /*-------*/

      (void) close( p[1] );
      status = 0;

      if (!(f = fdopen(p[0], "r")))
      {
	      Error("fdopen() failed");

        return -1;
      }

      if( mode == ZOO_FILE_MODE )
      {
	if( ReadTreeFromZOO( statistic.tree, f ) )
        {
	  Error("ReadTreeFromZOO() failed");
          (void) fclose( f );
	  (void) wait( &status );
          return( -1 );
	}
      }
      else if( mode == RPM_FILE_MODE )
      {
	if( ReadTreeFromRPM( statistic.tree, f ) )
        {
	  Error("ReadTreeFromRPM() failed");
          (void) fclose( f );
	  (void) wait( &status );
          return( -1 );
	}
      }
      else if( mode == LHA_FILE_MODE )
      {
	if( ReadTreeFromLHA( statistic.tree, f ) )
        {
	  Error("ReadTreeFromLHA() failed");
          (void) fclose( f );
	  (void) wait( &status );
          return( -1 );
	}
      }
      else if( mode == ZIP_FILE_MODE )
      {
	if( ReadTreeFromZIP( statistic.tree, f ) )
        {
	  Error("ReadTreeFromZIP() failed");
          (void) fclose( f );
	  (void) wait( &status );
          return( -1 );
	}
      }
      else if( mode == ARC_FILE_MODE )
      {
	if( ReadTreeFromARC( statistic.tree, f ) )
        {
	  Error("ReadTreeFromARC() failed");
          (void) fclose( f );
	  (void) wait( &status );
          return( -1 );
	}
      }
      else if( mode == RAR_FILE_MODE )
      {
	if( ReadTreeFromRAR( statistic.tree, f ) )
        {
	  Error("ReadTreeFromRAR() failed");
          (void) fclose( f );
	  (void) wait( &status );
          return( -1 );
	}
      }
      else
      {
        if( ReadTreeFromTAR( statistic.tree, f ) )
        {
          Error("ReadTreeFromTAR() failed");
          (void) fclose( f );
	  (void) wait( &status );
          return( -1 );
        }
      }
      (void) wait( &status );
      if(status)
      {
        MessagePrintf("ReadTarFile() failed*can't execute*%s", command_line);
      }
      (void) fclose( f );
    }
  }
  else
  {
    if( *disk_statistic.login_path )
    {
      /* Alten Baum loeschen */
      /*---------------------*/
      *disk_statistic.login_path = '\0';
      DeleteTree( disk_statistic.tree );
    }

    (void) strcpy( statistic.tree->name, path );
    statistic.tree->next = statistic.tree->prev = NULL;

    depth = strtod(TREEDEPTH, NULL);
    if (ReadTree(statistic.tree, path, depth))
    {
      Error("ReadTree() failed");

      return -1;
    }
    (void) memcpy( (char *) &disk_statistic,
		   (char *) &statistic,
		   sizeof( Statistic )
		 );
  }

  (void) SetFileSpec( statistic.file_spec );
/*  SetKindOfSort( statistic.kind_of_sort ); */

  return( 0 );
}





int GetNewLoginPath(char *path)
{
  int result;
  char *cptr;
  char aux[PATH_LENGTH * 2 + 1]= "";

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "NEW LOGIN-PATH:" );

  strcpy(aux,path);
  if( mode == LL_FILE_MODE && *path == '<' )
  {
    for( cptr = aux; (*cptr = *(cptr + 1)); cptr++ )
      ;
    if( aux[strlen(aux) - 1] == '>' ) aux[strlen(aux) - 1 ] = '\0';
  }

  if( InputString( aux, LINES - 2, 17, 0, COLS - 24, "\r\033" ) == CR )
  {
    NormPath(aux, path);
    result = 0;
  }

return( result );
}


