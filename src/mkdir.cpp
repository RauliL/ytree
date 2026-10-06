/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/mkdir.c,v 1.17 2005/01/22 16:32:29 werner Exp $
 *
 * Erstellen von Verzeichnissen
 *
 ***************************************************************************/


#include "ytree.h"

#include <filesystem>
#include <system_error>




int MakeDirectory(DirEntry *father_dir_entry)
{
  char dir_name[PATH_LENGTH * 2 +1];
  int result = -1;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Make Subdirectory: " );

  *dir_name = '\0';

  if (InputString( dir_name, LINES - 2, 20, 0, COLS - 20 - 1) == CR)
  {
    result = MakeDirEntry( father_dir_entry, dir_name );
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}




int MakeDirEntry(DirEntry *father_dir_entry, char *dir_name )
{
  DirEntry *den_ptr, *des_ptr;
  struct stat stat_struct;
  int result = -1;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  const auto path =
    std::filesystem::path(GetPath(father_dir_entry)) / dir_name;
  const std::string path_str = path.string();

  std::error_code ec;
  if( !std::filesystem::create_directory( path, ec ) )
  {
    if( !ec )
      ec = std::make_error_code( std::errc::file_exists );
    FormatMessage("Can't create Directory*\"{}\"*{}", path_str.c_str(), ec.message().c_str());
  }
  else
  {
    const auto perms = static_cast<std::filesystem::perms>(
      (S_IRWXU | S_IRWXG | S_IRWXO) & ~user_umask
    );
    std::filesystem::permissions( path, perms, ec );
    if( ec )
    {
      FormatWarning("Can't chmod Directory*\"{}\"*{}*IGNORED", path_str.c_str(), ec.message().c_str());
    }

    /* Directory erstellt
     * ==> einklinken im Baum
     */
    den_ptr = MallocOrAbort<DirEntry>(sizeof(DirEntry) + std::strlen(dir_name));
    den_ptr->file = nullptr;
    den_ptr->next = nullptr;
    den_ptr->prev = nullptr;
    den_ptr->sub_tree = nullptr;
    den_ptr->total_bytes    = 0L;
    den_ptr->matching_bytes = 0L;
    den_ptr->tagged_bytes   = 0L;
    den_ptr->total_files    = 0;
    den_ptr->matching_files = 0;
    den_ptr->tagged_files   = 0;
    den_ptr->access_denied  = false;
    den_ptr->cursor_pos     = 0;
    den_ptr->start_file     = 0;
    den_ptr->global_flag    = false;
    den_ptr->login_flag     = false;
    den_ptr->big_window     = false;
    den_ptr->up_tree = father_dir_entry;
    den_ptr->not_scanned    = false;

    statistic.disk_total_directories++;

    std::strcpy(den_ptr->name, dir_name);

    StatOrAbort(path_str, stat_struct);

    std::memcpy(
      static_cast<void*>(&den_ptr->stat_struct),
      static_cast<const void*>(&stat_struct),
      sizeof(stat_struct)
    );

    /* Sortieren durch direktes Einfuegen */
    /*------------------------------------*/

    for( des_ptr = father_dir_entry->sub_tree; des_ptr; des_ptr = des_ptr->next )
    {
      if( strcmp( des_ptr->name, den_ptr->name ) > 0 )
      {
	/* des-Element ist groesser */
	/*--------------------------*/

	den_ptr->next = des_ptr;
	den_ptr->prev = des_ptr->prev;
	if( des_ptr->prev) des_ptr->prev->next = den_ptr;
	else father_dir_entry->sub_tree = den_ptr;
	des_ptr->prev = den_ptr;
	break;
      }

      if( des_ptr->next == nullptr )
      {
        /* Ende der Liste erreicht; ==> einfuegen */
        /*----------------------------------------*/

        den_ptr->prev = des_ptr;
	den_ptr->next = des_ptr->next;
        des_ptr->next = den_ptr;
	break;
      }
    }

    if( father_dir_entry->sub_tree == nullptr )
    {
      /* Erstes Element */
      /*----------------*/

      father_dir_entry->sub_tree = den_ptr;
      den_ptr->prev = nullptr;
      den_ptr->next = nullptr;
    }

    (void) GetAvailBytes( &statistic.disk_space );

    result = 0;
  }

  return( result );
}



int MakePath( DirEntry *tree, char *dir_path, DirEntry **dest_dir_entry )
{
  DirEntry *de_ptr, *sde_ptr;
  char     path[PATH_LENGTH+1];
  char     *token, *old;
  int      n;
  int      result = -1;

  NormPath( dir_path, path );
  *dest_dir_entry = nullptr;

  n = strlen( tree->name );
  const char preferred_separator_str[]{
    std::filesystem::path::preferred_separator, '\0'};
  const auto tree_is_root =
    tree->name[0] == std::filesystem::path::preferred_separator &&
    tree->name[1] == '\0';

  if( tree_is_root ||
      ( !strncmp( tree->name, path, n ) &&
       ( path[n] == std::filesystem::path::preferred_separator || path[n] == '\0' ) ) )
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree;
    token = Strtok_r( &path[n], preferred_separator_str, &old );
    while( token )
    {
      for( sde_ptr = de_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next )
      {
        if( !strcmp( sde_ptr->name, token ) )
	{
	  /* Subtree gefunden */
	  /*------------------*/

	  de_ptr = sde_ptr;
	  break;
	}
      }
      if( sde_ptr == nullptr )
      {
	/* Folgeverzeichnis nicht vorhanden */
	/*----------------------------------*/

#ifdef DEBUG
  fprintf( stderr, "MakeDirEntry: \"%s\"\n", token );
#endif /* DEBUG */

	if( MakeDirEntry( de_ptr, token ) )
	{
	  return( result );
	}
	continue;
      }
      token = Strtok_r( nullptr, preferred_separator_str, &old );
    }
    *dest_dir_entry = de_ptr;
    result = 0;
  }
  else
  {
    /* Zielverzeichnis ist nicht im Subtree */
    /*--------------------------------------*/

#ifdef DEBUG
    fprintf( stderr, "MakePath: \"%s\"\n", path );
#endif /* DEBUG */

    std::error_code ec;
    std::filesystem::create_directories( path, ec );
    /* Old progressive mkdir broke on non-EEXIST errors but still fell
     * through to result = 0; preserve that return value for compatibility.
     * create_directories treats an existing directory as success (!ec). */
    (void)ec;
    result = 0;
  }

  return( result );
}


