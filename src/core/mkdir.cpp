#include <algorithm>

#include "core/ui.hpp"
#include "core/api.hpp"
#include "core/path.hpp"
#include "core/stat-macros.hpp"
#include "util/string-tokenizer.hpp"

static bool MakeDirEntry(DirEntry*, const std::string&);

std::optional<std::string> MakeDirectory(DirEntry *father_dir_entry)
{
  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    ui::Beep();
    return std::nullopt;
  }

  const auto name = ui::Prompt("Make Subdirectory:");

  if (!name)
  {
    return std::nullopt;
  }

  if (!MakeDirEntry(father_dir_entry, *name))
  {
    return std::nullopt;
  }

  return name;
}

static bool MakeDirEntry(DirEntry *father_dir_entry, const std::string& dir_name)
{
  bool result = false;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    ui::Beep();
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
    den_ptr->parent = WeakDirEntry(father_dir_entry);
    den_ptr->name = dir_name;

    statistic.disk_total_directories++;

    den_ptr->meta = MetaOrAbort(path_str);

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

    GetAvailBytes(statistic.disk_space);

    result = true;
  }

  return( result );
}



bool MakePath(const std::shared_ptr<DirEntry>& tree, const std::string& dir_path, DirEntry **dest_dir_entry)
{
  DirEntry *de_ptr, *sde_ptr;
  int n;
  bool result = false;

  auto path = NormPath(dir_path);
  *dest_dir_entry = nullptr;

  n = tree->name.size();
  const char preferred_separator_str[]{
    std::filesystem::path::preferred_separator, '\0'};
  const auto tree_is_root = IsRoot(tree->name);

  if (tree_is_root ||
      (tree->name.compare(0, n, path, 0, n) == 0 &&
       (path[n] == std::filesystem::path::preferred_separator || path[n] == '\0')))
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree.get();
    StringTokenizer tokenizer(std::string_view(path).substr(n));

    while (const auto token = tokenizer.Next(preferred_separator_str))
    {
      sde_ptr = nullptr;
      for (const auto& child : de_ptr->children)
      {
        if (child->name == *token)
        {
          /* Subtree gefunden */
          /*------------------*/

          sde_ptr = child.get();
          de_ptr = sde_ptr;
          break;
        }
      }
      if (sde_ptr == nullptr)
      {
        /* Folgeverzeichnis nicht vorhanden */
        /*----------------------------------*/

#ifdef DEBUG
        std::fprintf(stderr, "MakeDirEntry: \"%.*s\"\n",
                     static_cast<int>(token->size()), token->data());
#endif /* DEBUG */

        if (!MakeDirEntry(de_ptr, std::string(*token)))
        {
          return result;
        }

        for (const auto& child : de_ptr->children)
        {
          if (child->name == *token)
          {
            de_ptr = child.get();
            break;
          }
        }
      }
    }
    *dest_dir_entry = de_ptr;
    result = true;
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
     * through to result = true; preserve that return value for compatibility.
     * create_directories treats an existing directory as success (!ec). */
    (void)ec;
    result = true;
  }

  return result;
}
