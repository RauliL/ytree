/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/archive.c,v 1.16 2000/05/20 20:41:11 werner Exp $
 *
 * Allg. Funktionen zum Bearbeiten von Archiven
 *
 ***************************************************************************/


#include "ytree.h"



static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry);


static int InsertArchiveDirEntry(DirEntry *tree, char *path, struct stat *stat)
{
  DirEntry *df_ptr, *de_ptr, *ds_ptr;
  char father_path[PATH_LENGTH + 1];
  char *p;
  char name[PATH_LENGTH + 1];


#ifdef DEBUG
  fprintf( stderr, "Insert Dir \"%s\"\n", path );
#endif

  /* Format: .../dir/ */
  /*------------------*/

  (void) strcpy( father_path, path );

  if( ( p = strrchr( father_path, FILE_SEPARATOR_CHAR ) ) ) *p = '\0';
  else
  {
    ErrorPrintf(
      "patch mismatch*missing '%c' in*%s",
      FILE_SEPARATOR_CHAR,
      path
    );

    return -1;
  }

  p = strrchr( father_path, FILE_SEPARATOR_CHAR );

  if( p == NULL )
  {
    df_ptr = tree;
    if( !strcmp( path, FILE_SEPARATOR_STRING ) )
      (void) strcpy( name, path );
    else
      (void) strcpy( name, father_path );
  }
  else
  {
    (void) strcpy( name, ++p );
    *p = '\0';
    if( GetArchiveDirEntry( tree, father_path, &df_ptr ) )
    {
      ErrorPrintf("can't find subdir*%s", father_path);

      return -1;
    }
  }

  de_ptr = MallocOrAbort<DirEntry>(sizeof(DirEntry) + std::strlen(name));

  (void) memset( (char *) de_ptr, 0, sizeof( DirEntry ) );
  (void) strcpy( de_ptr->name, name );
  (void) memcpy( (char *) &de_ptr->stat_struct, (char *) stat, sizeof( struct stat ) );

#ifdef DEBUG
  fprintf( stderr, "new dir: \"%s\"\n", name );
#endif



  /* Directory einklinken */
  /*----------------------*/

  if( p == NULL )
  {
    /* in tree (=df_ptr) einklinken */
    /*------------------------------*/

    de_ptr->up_tree = df_ptr->up_tree;

    for( ds_ptr = df_ptr; ds_ptr; ds_ptr = ds_ptr->next )
    {
      if( strcmp( ds_ptr->name, de_ptr->name ) > 0 )
      {
        /* ds-Element ist groesser */
        /*-------------------------*/

        de_ptr->next = ds_ptr;
        de_ptr->prev = ds_ptr->prev;
        if( ds_ptr->prev) ds_ptr->prev->next = de_ptr;
        ds_ptr->prev = de_ptr;
	if( de_ptr->up_tree && de_ptr->up_tree->sub_tree == de_ptr->next )
	  de_ptr->up_tree->sub_tree = de_ptr;
        break;
      }

      if( ds_ptr->next == NULL )
      {
        /* Ende der Liste erreicht; ==> einfuegen */
        /*----------------------------------------*/

        de_ptr->prev = ds_ptr;
        de_ptr->next = ds_ptr->next;
        ds_ptr->next = de_ptr;
        break;
      }
    }
  }
  else if( df_ptr->sub_tree == NULL )
  {
    de_ptr->up_tree = df_ptr;
    df_ptr->sub_tree = de_ptr;
  }
  else
  {
    de_ptr->up_tree = df_ptr;

    for( ds_ptr = df_ptr->sub_tree; ds_ptr; ds_ptr = ds_ptr->next )
    {
      if( strcmp( ds_ptr->name, de_ptr->name ) > 0 )
      {
        /* ds-Element ist groesser */
        /*-------------------------*/

        de_ptr->next = ds_ptr;
        de_ptr->prev = ds_ptr->prev;
        if( ds_ptr->prev ) ds_ptr->prev->next = de_ptr;
        ds_ptr->prev = de_ptr;
	if( de_ptr->up_tree->sub_tree == de_ptr->next )
	  de_ptr->up_tree->sub_tree = de_ptr;
        break;
      }

      if( ds_ptr->next == NULL )
      {
        /* Ende der Liste erreicht; ==> einfuegen */
        /*----------------------------------------*/

        de_ptr->prev = ds_ptr;
        de_ptr->next = ds_ptr->next;
        ds_ptr->next = de_ptr;
        break;
      }
    }
  }
  statistic.disk_total_directories++;
  return( 0 );
}





int InsertArchiveFileEntry(DirEntry *tree, char *path, struct stat *stat)
{
  char dir[PATH_LENGTH + 1];
  char file[PATH_LENGTH + 1];
  DirEntry *de_ptr;
  FileEntry *fs_ptr, *fe_ptr;
  struct stat stat_struct;
  int  n;


  if( KeyPressed() )
  {
    Quit();  /* Abfrage, ob ytree verlassen werden soll */
  }


  Fnsplit( path, dir, file );

  if( GetArchiveDirEntry( tree, dir, &de_ptr ) )
  {
#ifdef DEBUG
    fprintf( stderr, "can't get directory for file*%s*trying recover", path );
#endif

    (void) memset( (char *) &stat_struct, 0, sizeof( struct stat ) );
    stat_struct.st_mode = S_IFDIR;

    if( TryInsertArchiveDirEntry( tree, dir, &stat_struct ) )
    {
      Error("Inserting directory failed");

      return -1;
    }
    if( GetArchiveDirEntry( tree, dir, &de_ptr ) )
    {
      ErrorPrintf("again: can't get directory for file*%s*giving up", path);

      return -1;
    }
  }

  if( S_ISLNK( stat->st_mode ) )
    n = strlen( &path[ strlen( path ) + 1 ] ) + 1;
  else
    n = 0;

  fe_ptr = MallocOrAbort<FileEntry>(sizeof(FileEntry) + std::strlen(file) + n);

  (void) memset( fe_ptr, 0, sizeof( FileEntry ) );
  (void) memcpy( (char *) &fe_ptr->stat_struct, (char *) stat, sizeof( struct stat ) );
  (void) strcpy( fe_ptr->name, file );

  if( S_ISLNK( stat->st_mode ) )
  {
    (void) strcpy( &fe_ptr->name[ strlen( fe_ptr->name ) + 1 ],
		   &path[ strlen( path ) + 1 ]
		 );
  }

  fe_ptr->dir_entry = de_ptr;
  de_ptr->total_files++;
  de_ptr->total_bytes += stat->st_size;
  statistic.disk_total_files++;
  statistic.disk_total_bytes += stat->st_size;

  /* Einklinken */
  /*------------*/

  if( de_ptr->file == NULL )
  {
    de_ptr->file = fe_ptr;
  }
  else
  {
    for( fs_ptr = de_ptr->file; fs_ptr->next; fs_ptr = fs_ptr->next )
      ;

    fe_ptr->prev = fs_ptr;
    fs_ptr->next = fe_ptr;
  }
  return( 0 );
}





static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry)
{
  int n;
  DirEntry *de_ptr;
  bool is_root = false;

#ifdef DEBUG
  fprintf( stderr, "GetArchiveDirEntry: tree=%s, path=%s\n",
  (tree) ? tree->name : "NULL", path );
#endif

  if( strchr( path, FILE_SEPARATOR_CHAR ) != NULL )
  {
    for( de_ptr = tree; de_ptr; de_ptr = de_ptr->next )
    {
      n = strlen( de_ptr->name );
      if( !strcmp( de_ptr->name, FILE_SEPARATOR_STRING ) ) is_root = true;

      if( n && !strncmp( de_ptr->name, path, n ) &&
	  (is_root || path[n] == '\0' || path[n] == FILE_SEPARATOR_CHAR ) )
      {
	if( ( is_root && path[n] == '\0' ) ||
	    ( path[n] == FILE_SEPARATOR_CHAR && path[n+1] == '\0' ) )
	{
	  /* Pfad abgearbeitet; ==> fertig */
	  /*-------------------------------*/

	  *dir_entry = de_ptr;
	  return( 0 );
	}
	else
        {
	  return( GetArchiveDirEntry( de_ptr->sub_tree,
				  ( is_root ) ? &path[n] : &path[n+1],
				  dir_entry
				) );
	}
      }
    }
  }
  if( *path == '\0' )
  {
    *dir_entry = tree;
    return( 0 );
  }
  return( -1 );
}





int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat)
{
  DirEntry *de_ptr;
  char dir_path[PATH_LENGTH + 1];
  char *s, *t;

  (void) memset( dir_path, 0, sizeof( dir_path ) );

#ifdef DEBUG
  fprintf( stderr, "Try install start \n" );
#endif

  for( s=dir, t=dir_path; *s; s++, t++ )
  {
    if( (*t = *s) == FILE_SEPARATOR_CHAR )
    {
      if( GetArchiveDirEntry( tree, dir_path, &de_ptr ) == -1 )
      {
	/* Evtl. fehlender teil; ==> einfuegen */
	/*-------------------------------------*/

	if( InsertArchiveDirEntry( tree, dir_path, stat ) ) return( -1 );
      }
    }
  }

#ifdef DEBUG
  fprintf( stderr, "Try install end\n" );
#endif

  return( 0 );
}






int MinimizeArchiveTree(DirEntry *tree)
{
  DirEntry  *de_ptr, *de1_ptr;
  DirEntry  *next_ptr;
  FileEntry *fe_ptr;


  /* Falls tree einen Nachfolger hat und
   * tree selbst leer ist, wird tree gestrichen
   */

  if( tree->prev == NULL &&
      tree->next != NULL &&
      tree->file == NULL )
  {
    next_ptr = tree->next;
    (void) memcpy( (char *) tree,
		   (char *) tree->next,
		   sizeof( DirEntry ) + strlen( tree->next->name )
		 );
    tree->prev = NULL;
    if( tree->next ) tree->next->prev = tree;
    statistic.disk_total_directories--;
    free( next_ptr );
    for( fe_ptr=tree->file; fe_ptr; fe_ptr=fe_ptr->next)
      fe_ptr->dir_entry = tree;
    for( de_ptr=tree->sub_tree; de_ptr; de_ptr=de_ptr->next)
      de_ptr->up_tree = tree;
  }


  /* Test, ob *de_ptr weder Vorgaenger noch Nachfolger noch Dateien hat */
  /*--------------------------------------------------------------------*/

  for( de_ptr = tree->sub_tree; de_ptr; de_ptr = next_ptr )
  {
    if( de_ptr->prev == NULL && de_ptr->next == NULL && de_ptr->file == NULL )
    {
      /* Zusammenfassung moeglich */
      /*--------------------------*/

      if( strcmp( tree->name, FILE_SEPARATOR_STRING ) )
	(void) strcat( tree->name, FILE_SEPARATOR_STRING );
      (void) strcat( tree->name, de_ptr->name );
      statistic.disk_total_directories--;
      tree->sub_tree = de_ptr->sub_tree;
      for( de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next )
	de1_ptr->up_tree = tree;
      next_ptr = de_ptr->sub_tree;
      free( de_ptr );
#ifdef DEBUG
  fprintf( stderr, "new root-dir: \"%s\"\n", tree->name );
#endif
      continue;
    }
    break;
  }

  /* Letzter Optimierungsschritt:
   * Falls tree weder Vorgaenger noch Nachfolger hat, aber
   * einen Subtree der Files hat, wird zusammengefasst
   */

  if( tree->prev == NULL &&
      tree->next == NULL &&
      tree->file == NULL &&
      tree->sub_tree     &&
      tree->sub_tree->prev == NULL &&
      tree->sub_tree->next == NULL
    )
  {
    de_ptr = tree->sub_tree;
    (void) strcat( tree->name, FILE_SEPARATOR_STRING );
    (void) strcat( tree->name, de_ptr->name );
    tree->file = de_ptr->file;
    for( fe_ptr=tree->file; fe_ptr; fe_ptr=fe_ptr->next )
      fe_ptr->dir_entry = tree;
    (void) memcpy( (char *) &tree->stat_struct,
		   (char *) &de_ptr->stat_struct,
		   sizeof( struct stat )
		  );
    statistic.disk_total_directories--;
    tree->sub_tree = de_ptr->sub_tree;
    for( de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next )
      de1_ptr->up_tree = tree;
    free( de_ptr );
  }
  return( 0 );
}

void MakeExtractCommandLine(
  char* command_line,
  const std::size_t size,
  const std::string& path,
  const std::string& file,
  const std::string& cmd
)
{
  const auto compress_method = GetFileMethod(path);
  const auto l = path.length();
  char cat_path[PATH_LENGTH + 1];

  if (compress_method && *compress_method == CompressMethod::ZOO_COMPRESS)
  {
    /* zoo xp FILE ?? */
    /*----------------*/
    std::snprintf(
      command_line,
      size,
      "%s '%s' '%s' %s",
      ZOOEXPAND,
      path.c_str(),
      file.c_str(),
      cmd.c_str()
    );
  }
  else if (compress_method && *compress_method == CompressMethod::LHA_COMPRESS)
  {
    /* xlharc p FILE ?? */
    /*------------------*/
    std::snprintf(
      command_line,
      size,
      "%s '%s' '%s' %s",
      LHAEXPAND,
		  path.c_str(),
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::ZIP_COMPRESS)
  {
    /* unzip -c FILE ?? */
    /*------------------*/
    std::snprintf(
      command_line,
      size,
      "%s '%s' '%s' %s",
		  ZIPEXPAND,
		  path.c_str(),
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::ARC_COMPRESS)
  {
    /* arc p FILE ?? */
    /*---------------*/
    std::snprintf(
      command_line,
      size,
      "%s '%s' '%s' %s",
		  ARCEXPAND,
		  path.c_str(),
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::RPM_COMPRESS)
  {
    /* TF=/tmp/ytree.$$; mkdir $TF; cd $TF; rpm2cpio RPM_FILE | cpio -id FILE;
     * cat $TF/$2; cd /tmp; rm -rf $TF; exit 0
     */
    if (!std::strcmp(RPMEXPAND, "builtin"))
    {
      std::snprintf(
        command_line,
        size,
        "(TF=/tmp/ytree.$$; mkdir $TF; rpm2cpio '%s' | (cd $TF; cpio --no-absolute-filenames -i -d '%s'); cat \"$TF/%s\"; cd /tmp; rm -rf $TF; exit 0) %s",
		    path.c_str(),
        !file.empty() && file[0] == FILE_SEPARATOR_CHAR ? file.substr(1).c_str() : file.c_str(),
        file.c_str(),
        cmd.c_str()
		  );
    } else {
      std::snprintf(
        command_line,
        size,
        "%s '%s' '%s' %s",
		    RPMEXPAND,
		    path.c_str(),
		    file.c_str(),
		    cmd.c_str()
		  );
    }
  }
  else if (compress_method && *compress_method == CompressMethod::RAR_COMPRESS)
  {
    /* rar p FILE ?? */
    /*---------------*/
    std::snprintf(
      command_line,
      size,
      "%s '%s' '%s' %s",
		  RAREXPAND,
		  path.c_str(),
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::FREEZE_COMPRESS)
  {
    /* melt < TAR_FILE | gtar xOf - FILE ?? */
    /*--------------------------------------*/
    std::snprintf(
      command_line,
      size,
      "%s < '%s' | %s '%s' %s",
		  MELT,
		  path.c_str(),
		  TAREXPAND,
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::MULTIPLE_FREEZE_COMPRESS)
  {
    /* CAT TAR_FILEs | melt | gtar xOf - FILE ?? */
    /*-------------------------------------------*/
    std::strncpy(cat_path, path.c_str(), l - 2 );
    std::strcpy(&cat_path[l - 2], "*" );
    std::snprintf(
      command_line,
      size,
      "%s %s | %s | %s '%s' %s",
		  CAT,
		  cat_path,
		  MELT,
		  TAREXPAND,
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::COMPRESS_COMPRESS)
  {
    /* uncompress < TAR_FILE | gtar xOf - FILE ?? */
    /*--------------------------------------------*/
    std::snprintf(
      command_line,
      size,
      "%s < %s | %s '%s' %s",
		  UNCOMPRESS,
      path.c_str(),
		  TAREXPAND,
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::MULTIPLE_COMPRESS_COMPRESS)
  {
    /* CAT TAR_FILEs | uncompress | gtar xOf - FILE ?? */
    /*-------------------------------------------------*/
    std::strncpy(cat_path, path.c_str(), l - 2);
    std::strcpy(&cat_path[l-2], "*");
    std::snprintf(
      command_line,
      size,
      "%s %s | %s | %s '%s' %s",
		  CAT,
		  cat_path,
		  UNCOMPRESS,
		  TAREXPAND,
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::GZIP_COMPRESS)
  {
    /* gunzip < TAR_FILE | gtar xOf - FILE ?? */
    /*----------------------------------------*/
    std::snprintf(
      command_line,
      size,
      "%s < '%s' | %s '%s' %s",
		  GNUUNZIP,
		  path.c_str(),
		  TAREXPAND,
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::MULTIPLE_GZIP_COMPRESS)
  {
    /* CAT TAR_FILEs | gunzip | gtar xOf - FILE ?? */
    /*---------------------------------------------*/
    std::strncpy(cat_path, path.c_str(), l - 2);
    std::strcpy(&cat_path[l-2], "*");
    std::snprintf(
      command_line,
      size,
      "%s %s | %s | %s '%s' %s",
		  CAT,
		  cat_path,
		  GNUUNZIP,
		  TAREXPAND,
		  file.c_str(),
		  cmd.c_str()
		);
  }
  else if (compress_method && *compress_method == CompressMethod::BZIP_COMPRESS)
  {
    /* bunzip2 < TAR_FILE | gtar xOf - FILE ?? */
    /*----------------------------------------*/
    std::snprintf(
      command_line,
      size,
      "%s < '%s' | %s '%s' %s",
		  BUNZIP,
		  path.c_str(),
		  TAREXPAND,
		  file.c_str(),
		  cmd.c_str()
		);
  } else {
    /* gtar xOf - FILE < TAR_FILE ?? */
    /*-------------------------------*/
    std::snprintf(
      command_line,
      size,
      "%s '%s' < '%s' %s",
		  TAREXPAND,
		  file.c_str(),
		  path.c_str(),
		  cmd.c_str()
		);
  }

#ifdef DEBUG
  fprintf( stderr, "system( \"%s\" )\n", command_line );
#endif
}
