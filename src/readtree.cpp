#include <algorithm>

#include "./ytree.hpp"

static void UnReadSubTree(DirEntry *dir_entry);

/* Dateibaum lesen: path = "Root"-Pfad
 * dir_entry wird von der Funktion gefuellt
 */
int ReadTree(
  const std::shared_ptr<DirEntry>& dir_entry,
  const std::string& path,
  int depth
)
{
  int   file_count;
  std::vector<std::shared_ptr<DirEntry>> new_children;
  std::vector<std::shared_ptr<FileEntry>> new_files;

  /* dir_entry initialisieren */
  /*--------------------------*/

  dir_entry->files.clear();
  dir_entry->children.clear();
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

  if (dir_entry->meta.IsBlock())
  {
    return 0; // Block device
  }

  if (depth < 0)
  {
    if (const auto parent = dir_entry->Parent())
    {
      parent->not_scanned = true;

      return 1;
    }
  }

  statistic.disk_total_directories++;

  std::error_code ec;
  std::filesystem::directory_iterator it(path, ec);
  std::filesystem::directory_iterator end;

  if (ec)
  {
    dir_entry->access_denied = true;

    return 1;
  }

  file_count = 0;

  for (; it != end; it.increment(ec))
  {
    if (ec)
    {
      break;
    }

    auto entry_name = it->path().filename().string();
    std::string new_path;

    if (entry_name == "." || entry_name == "..")
    {
      continue;
    }

    if (EscapeKeyPressed())
    {
      Quit();  /* Abfrage ob ytree verlassen werden soll */
    }

    if ((file_count++ % 100) == 0)
    {
      DisplayDiskStatistic();
      doupdate();
    }

    new_path = it->path();

    std::error_code ec;
    auto entry_meta = EntryMeta::FromFilesystem(new_path, ec);
    if (!entry_meta)
    {
      if (ec.value() != EACCES)
      {
        FormatError("stat() failed on*{}*IGNORED", new_path);
      }
      continue;
    }

    if (entry_meta->IsDir())
    {
      /* Directory-Entry */
      /*-----------------*/
      auto den_ptr = std::make_shared<DirEntry>();
      den_ptr->parent = dir_entry;
      den_ptr->name = entry_name;
      den_ptr->meta = *entry_meta;

      ReadTree(den_ptr, new_path, depth - 1);

      new_children.push_back(den_ptr);
    } else {
      /* File-Entry */
      /*------------*/
      auto fen_ptr = std::make_shared<FileEntry>();

      /* Test, ob Eintrag Symbolischer Link ist */
      /*----------------------------------------*/

      fen_ptr->name = entry_name;
      fen_ptr->meta = *entry_meta;

      if (entry_meta->IsLink())
      {
        char link_path[PATH_LENGTH + 1];
        const auto n = readlink(new_path.c_str(), link_path, sizeof(link_path));

        /* Ja, symbolischer Name wird an "echten" Namen angehaengt */
        /*---------------------------------------------------------*/
        if (n == -1)
        {
          fen_ptr->symlink_target = "unknown";
        } else {
          fen_ptr->symlink_target.assign(link_path, static_cast<std::size_t>(n));
        }
      }

      fen_ptr->dir_entry = dir_entry;
      new_files.push_back(fen_ptr);
      dir_entry->total_files++;
      dir_entry->total_bytes += entry_meta->size;
      statistic.disk_total_files++;
      statistic.disk_total_bytes += entry_meta->size;
    }
  }

  /* Sortieren der Unterverzeichnisse (stabil, nach Namen) */
  /*-------------------------------------------------------*/

  std::stable_sort(
    new_children.begin(),
    new_children.end(),
    [](const std::shared_ptr<DirEntry>& a, const std::shared_ptr<DirEntry>& b)
    {
      return a->name < b->name;
    }
  );

  dir_entry->files = std::move(new_files);
  dir_entry->children = std::move(new_children);

  DisplayDiskStatistic();
  doupdate();

  return 0;
}

static void RemoveAllFiles(DirEntry* dir_entry)
{
  while (!dir_entry->files.empty())
  {
    dir_entry->files.back()->Remove();
  }
}


void UnReadTree(DirEntry *dir_entry)
{
  if( dir_entry == statistic.tree.get() )
  {
    Message("Can't delete ROOT");
  }
  else
  {
    RemoveAllFiles(dir_entry);
    UnReadSubTree(dir_entry);
    statistic.disk_total_directories--;
    GetAvailBytes(&statistic.disk_space);
    DisplayDiskStatistic();
    doupdate();
  }
}

/* Loescht alle Unterverzeichnisse von parent */
/*--------------------------------------------*/
static void UnReadSubTree(DirEntry *parent)
{
  auto children = std::move(parent->children);

  parent->children.clear();

  for (const auto& de_ptr : children)
  {
    RemoveAllFiles(de_ptr.get());
    UnReadSubTree(de_ptr.get());

    if (!parent->not_scanned)
    {
      --statistic.disk_total_directories;
    }
    // de_ptr is released when "children" goes out of scope.
  }
}
