/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/archive.c,v 1.16 2000/05/20 20:41:11 werner Exp $
 *
 * Allg. Funktionen zum Bearbeiten von Archiven
 *
 ***************************************************************************/


#include "ytree.h"



static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry);


static int InsertArchiveDirEntry(const std::shared_ptr<DirEntry>& tree, char *path, struct stat *stat)
{
  DirEntry *df_ptr;
  char father_path[PATH_LENGTH + 1];
  char *p;
  char name[PATH_LENGTH + 1];


#ifdef DEBUG
  fprintf( stderr, "Insert Dir \"%s\"\n", path );
#endif

  /* Format: .../dir/ */
  /*------------------*/

  *std::format_to(father_path, "{}", path) = '\0';

  if( ( p = strrchr( father_path, std::filesystem::path::preferred_separator ) ) ) *p = '\0';
  else
  {
    FormatError("patch mismatch*missing '{}' in*{}", std::filesystem::path::preferred_separator, path);

    return -1;
  }

  p = strrchr( father_path, std::filesystem::path::preferred_separator );

  if( p == nullptr )
  {
    df_ptr = tree.get();
    if( path[0] == std::filesystem::path::preferred_separator && path[1] == '\0' )
      *std::format_to(name, "{}", path) = '\0';
    else
      *std::format_to(name, "{}", father_path) = '\0';
  }
  else
  {
    *std::format_to(name, "{}", ++p) = '\0';
    *p = '\0';
    if( GetArchiveDirEntry( tree.get(), father_path, &df_ptr ) )
    {
      FormatError("can't find subdir*{}", father_path);

      return -1;
    }
  }

  auto de_ptr = std::make_shared<DirEntry>();
  de_ptr->name = name;
  de_ptr->stat_struct = *stat;

#ifdef DEBUG
  fprintf( stderr, "new dir: \"%s\"\n", name );
#endif



  /* Directory einklinken */
  /*----------------------*/

  /* Entweder direkt in tree (= df_ptr) oder in dessen Unterverzeichnis
   * (= df_ptr); die Unterverzeichnisse bleiben dabei nach Namen sortiert.
   */
  de_ptr->parent = df_ptr->weak_from_this();

  df_ptr->children.insert(
    std::find_if(
      df_ptr->children.begin(),
      df_ptr->children.end(),
      [&de_ptr](const std::shared_ptr<DirEntry>& ds_ptr) { return ds_ptr->name > de_ptr->name; }
    ),
    de_ptr
  );

  statistic.disk_total_directories++;
  return( 0 );
}





int InsertArchiveFileEntry(const std::shared_ptr<DirEntry>& tree, char *path, struct stat *stat)
{
  char dir[PATH_LENGTH + 1];
  char file[PATH_LENGTH + 1];
  DirEntry *de_ptr;
  struct stat stat_struct;


  if( KeyPressed() )
  {
    Quit();  /* Abfrage, ob ytree verlassen werden soll */
  }


  Fnsplit( path, dir, file );

  if( GetArchiveDirEntry( tree.get(), dir, &de_ptr ) )
  {
#ifdef DEBUG
    fprintf( stderr, "can't get directory for file*%s*trying recover", path );
#endif

    memset( (char *) &stat_struct, 0, sizeof( struct stat ) );
    stat_struct.st_mode = S_IFDIR;

    if( TryInsertArchiveDirEntry( tree, dir, &stat_struct ) )
    {
      Error("Inserting directory failed");

      return -1;
    }
    if( GetArchiveDirEntry( tree.get(), dir, &de_ptr ) )
    {
      FormatError("again: can't get directory for file*{}*giving up", path);

      return -1;
    }
  }

  auto fe_ptr = std::make_shared<FileEntry>();
  fe_ptr->stat_struct = *stat;
  fe_ptr->name = file;

  if( S_ISLNK( stat->st_mode ) )
  {
    fe_ptr->symlink_target = &path[std::strlen(path) + 1];
  }

  fe_ptr->dir_entry = de_ptr->weak_from_this();
  de_ptr->total_files++;
  de_ptr->total_bytes += stat->st_size;
  statistic.disk_total_files++;
  statistic.disk_total_bytes += stat->st_size;

  /* Einklinken */
  /*------------*/

  de_ptr->files.push_back( fe_ptr );
  return( 0 );
}





static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry)
{
  int n;
  bool is_root = false;

#ifdef DEBUG
  fprintf( stderr, "GetArchiveDirEntry: tree=%s, path=%s\n",
  (tree) ? tree->name : "NULL", path );
#endif

  if( strchr( path, std::filesystem::path::preferred_separator ) != nullptr )
  {
    for( const auto& child_ptr : tree->children )
    {
      DirEntry *de_ptr = child_ptr.get();
      n = de_ptr->name.size();
      if( de_ptr->name.size() == 1 &&
          de_ptr->name[0] == std::filesystem::path::preferred_separator )
        is_root = true;

      if( n && de_ptr->name.compare(0, n, path, n) == 0 &&
	  (is_root || path[n] == '\0' || path[n] == std::filesystem::path::preferred_separator ) )
      {
	if( ( is_root && path[n] == '\0' ) ||
	    ( path[n] == std::filesystem::path::preferred_separator && path[n+1] == '\0' ) )
	{
	  /* Pfad abgearbeitet; ==> fertig */
	  /*-------------------------------*/

	  *dir_entry = de_ptr;
	  return( 0 );
	}
	else
        {
	  return( GetArchiveDirEntry( de_ptr,
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





int TryInsertArchiveDirEntry(const std::shared_ptr<DirEntry>& tree, char *dir, struct stat *stat)
{
  DirEntry *de_ptr;
  char dir_path[PATH_LENGTH + 1];
  char *s, *t;

  memset( dir_path, 0, sizeof( dir_path ) );

#ifdef DEBUG
  fprintf( stderr, "Try install start \n" );
#endif

  for( s=dir, t=dir_path; *s; s++, t++ )
  {
    if( (*t = *s) == std::filesystem::path::preferred_separator )
    {
      if( GetArchiveDirEntry( tree.get(), dir_path, &de_ptr ) == -1 )
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






int MinimizeArchiveTree(const std::shared_ptr<DirEntry>& tree)
{
  /* tree ist ein namenloser Platzhalter, dessen Unterverzeichnisse die
   * obersten Verzeichnisse des Archivs sind.
   *
   * Falls tree genau ein Unterverzeichnis hat und tree selbst keine Dateien
   * enthaelt, wird tree durch dieses Unterverzeichnis ersetzt. Bei mehreren
   * obersten Verzeichnissen (oder Dateien in tree) bleibt der Platzhalter
   * als Wurzel bestehen.
   */

  if( tree->files.empty() && tree->children.size() == 1 )
  {
    /* Keep the child alive while it is copied over its former owner. */
    const std::shared_ptr<DirEntry> child = tree->children.front();

    *tree = *child;
    tree->parent.reset();
    statistic.disk_total_directories--;
    for( const auto& fe_ptr : tree->files )
      fe_ptr->dir_entry = tree;
    for( const auto& de_ptr : tree->children )
      de_ptr->parent = tree;
  }
  else
  {
    return( 0 );
  }


  /* Test, ob das einzige Unterverzeichnis keine Dateien hat */
  /*---------------------------------------------------------*/

  while( tree->children.size() == 1 && tree->children.front()->files.empty() )
  {
    /* Zusammenfassung moeglich */
    /*--------------------------*/

    const std::shared_ptr<DirEntry> de_ptr = tree->children.front();

    if( !(tree->name.size() == 1 &&
          tree->name[0] == std::filesystem::path::preferred_separator) )
    {
      tree->name += std::filesystem::path::preferred_separator;
    }
    tree->name += de_ptr->name;
    statistic.disk_total_directories--;
    tree->children = std::move(de_ptr->children);
    for( const auto& de1_ptr : tree->children )
      de1_ptr->parent = tree;
#ifdef DEBUG
  fprintf( stderr, "new root-dir: \"%s\"\n", tree->name.c_str() );
#endif
  }

  /* Letzter Optimierungsschritt:
   * Falls tree keine Dateien, aber genau einen Subtree hat, wird
   * zusammengefasst
   */

  if( tree->files.empty() && tree->children.size() == 1 )
  {
    const std::shared_ptr<DirEntry> de_ptr = tree->children.front();

    tree->name += std::filesystem::path::preferred_separator;
    tree->name += de_ptr->name;
    tree->files = std::move(de_ptr->files);
    for( const auto& fe_ptr : tree->files )
      fe_ptr->dir_entry = tree;
    tree->stat_struct = de_ptr->stat_struct;
    statistic.disk_total_directories--;
    tree->children = std::move(de_ptr->children);
    for( const auto& de1_ptr : tree->children )
      de1_ptr->parent = tree;
  }
  return( 0 );
}

std::string MakeExtractCommandLine(
  const std::string& path,
  const std::string& file,
  const std::string& cmd
)
{
  char command_line[COMMAND_LINE_LENGTH + 1];
  const auto compress_method = GetFileMethod(path);
  const auto l = path.length();

  if (compress_method && *compress_method == CompressMethod::ZOO_COMPRESS)
  {
    /* zoo xp FILE ?? */
    /*----------------*/
    std::snprintf(
      command_line,
      COMMAND_LINE_LENGTH,
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
      COMMAND_LINE_LENGTH,
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
      COMMAND_LINE_LENGTH,
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
      COMMAND_LINE_LENGTH,
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
        COMMAND_LINE_LENGTH,
        "(TF=/tmp/ytree.$$; mkdir $TF; rpm2cpio '%s' | (cd $TF; cpio --no-absolute-filenames -i -d '%s'); cat \"$TF/%s\"; cd /tmp; rm -rf $TF; exit 0) %s",
		    path.c_str(),
        !file.empty() && file[0] == std::filesystem::path::preferred_separator ? file.substr(1).c_str() : file.c_str(),
        file.c_str(),
        cmd.c_str()
		  );
    } else {
      std::snprintf(
        command_line,
        COMMAND_LINE_LENGTH,
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
      COMMAND_LINE_LENGTH,
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
      COMMAND_LINE_LENGTH,
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
    const auto cat = path.substr(0, l - 2) + "*";
    std::snprintf(
      command_line,
      COMMAND_LINE_LENGTH,
      "%s %s | %s | %s '%s' %s",
		  CAT,
		  cat.c_str(),
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
      COMMAND_LINE_LENGTH,
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
    const auto cat = path.substr(0, l - 2) + "*";
    std::snprintf(
      command_line,
      COMMAND_LINE_LENGTH,
      "%s %s | %s | %s '%s' %s",
		  CAT,
		  cat.c_str(),
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
      COMMAND_LINE_LENGTH,
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
    const auto cat = path.substr(0, l - 2) + "*";
    std::snprintf(
      command_line,
      COMMAND_LINE_LENGTH,
      "%s %s | %s | %s '%s' %s",
		  CAT,
		  cat.c_str(),
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
      COMMAND_LINE_LENGTH,
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
      COMMAND_LINE_LENGTH,
      "%s '%s' < '%s' %s",
		  TAREXPAND,
		  file.c_str(),
		  path.c_str(),
		  cmd.c_str()
		);
  }

#ifdef DEBUG
  std::fprintf(stderr, "system(\"%s\");\n", command_line);
#endif

  return command_line;
}
