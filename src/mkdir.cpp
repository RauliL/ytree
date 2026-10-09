/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/mkdir.c,v 1.17 2005/01/22 16:32:29 werner Exp $
 *
 * Erstellen von Verzeichnissen
 *
 ***************************************************************************/
#include "ytree.h"

std::optional<std::string> MakeDirectory(DirEntry *father_dir_entry)
{
  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return std::nullopt;
  }

  ClearHelp();

  MvAddStr(LINES - 2, 1, "Make Subdirectory: ");

  const auto name = InputString({}, LINES - 2, 20, 0, COLS - 20 - 1);
  move(LINES - 2, 1);
  clrtoeol();

  if (!name)
  {
    return std::nullopt;
  }

  if (MakeDirEntry(father_dir_entry, *name) != 0)
  {
    return std::nullopt;
  }

  return name;
}




int MakeDirEntry(DirEntry *father_dir_entry, const std::string& dir_name)
{
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
  if( !std::filesystem::create_directory(path, ec) )
  {
    if( !ec )
      ec = std::make_error_code(std::errc::file_exists);
    FormatMessage("Can't create Directory*\"{}\"*{}", path_str.c_str(), ec.message().c_str());
  }
  else
  {
    const auto perms = static_cast<std::filesystem::perms>(
      (S_IRWXU | S_IRWXG | S_IRWXO) & ~user_umask
    );
    std::filesystem::permissions(path, perms, ec);
    if( ec )
    {
      FormatWarning("Can't chmod Directory*\"{}\"*{}*IGNORED", path_str.c_str(), ec.message().c_str());
    }

    /* Directory erstellt
     * ==> einklinken im Baum
     */
    auto den_ptr = std::make_shared<DirEntry>();
    den_ptr->parent = father_dir_entry->weak_from_this();
    den_ptr->name = dir_name;

    statistic.disk_total_directories++;

    StatOrAbort(path_str, stat_struct);
    den_ptr->stat_struct = stat_struct;

    /* Sortieren durch direktes Einfuegen */
    /*------------------------------------*/

    auto& siblings = father_dir_entry->children;

    siblings.insert(
      std::find_if(
        siblings.begin(),
        siblings.end(),
        [&den_ptr](const std::shared_ptr<DirEntry>& des_ptr) { return des_ptr->name > den_ptr->name; }
),
      den_ptr
);

    GetAvailBytes(&statistic.disk_space);

    result = 0;
  }

  return( result );
}



int MakePath(const std::shared_ptr<DirEntry>& tree, const std::string& dir_path, DirEntry **dest_dir_entry)
{
  DirEntry *de_ptr, *sde_ptr;
  char     *token, *old;
  int      n;
  int      result = -1;

  auto path = NormPath(dir_path);
  *dest_dir_entry = nullptr;

  n = tree->name.size();
  const char preferred_separator_str[]{
    std::filesystem::path::preferred_separator, '\0'};
  const auto tree_is_root =
    tree->name.size() == 1 &&
    tree->name[0] == std::filesystem::path::preferred_separator;

  if( tree_is_root ||
      ( tree->name.compare(0, n, path, 0, n) == 0 &&
       ( path[n] == std::filesystem::path::preferred_separator || path[n] == '\0' ) ) )
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree.get();
    token = Strtok_r(path.data() + n, preferred_separator_str, &old);
    while( token )
    {
      sde_ptr = nullptr;
      for( const auto& child : de_ptr->children )
      {
        if( child->name == token )
  {
    /* Subtree gefunden */
    /*------------------*/

    sde_ptr = child.get();
    de_ptr = sde_ptr;
    break;
  }
      }
      if( sde_ptr == nullptr )
      {
  /* Folgeverzeichnis nicht vorhanden */
  /*----------------------------------*/

#ifdef DEBUG
  std::fprintf(stderr, "MakeDirEntry: \"%s\"\n", token);
#endif /* DEBUG */

  if( MakeDirEntry(de_ptr, token) )
  {
    return( result );
  }
  continue;
      }
      token = Strtok_r(nullptr, preferred_separator_str, &old);
    }
    *dest_dir_entry = de_ptr;
    result = 0;
  }
  else
  {
    /* Zielverzeichnis ist nicht im Subtree */
    /*--------------------------------------*/

#ifdef DEBUG
    std::fprintf(stderr, "MakePath: \"%s\"\n", path.c_str());
#endif /* DEBUG */

    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    /* Old progressive mkdir broke on non-EEXIST errors but still fell
     * through to result = 0; preserve that return value for compatibility.
     * create_directories treats an existing directory as success (!ec). */
    (void)ec;
    result = 0;
  }

  return( result );
}


