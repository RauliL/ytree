#include "ytree.hpp"

static bool Move(const std::string&, const std::string&);

bool MoveFile(
  FileEntry* fe_ptr,
  bool confirm,
  const std::string& to_file,
  DirEntry* dest_dir_entry,
  const std::string& to_dir_path,
  FileEntry** new_fe_ptr
)
{
  const auto de_ptr = fe_ptr->Dir();
  const auto from_path =
    (std::filesystem::path(GetPath(de_ptr.get())) / fe_ptr->name).string();
  std::int64_t file_size;
  FileEntry   *dest_file_entry;
  int         term;
  bool        result = false;
  *new_fe_ptr = nullptr;

  const ScopeGuard clear_prompt_lines([] { ClearBottomPromptLines(); });

  const auto to_path =
    (std::filesystem::path(to_dir_path) / to_file).string();

  if (to_path == from_path)
  {
    Message("Can't move file into itself");
    return result;
  }

  if (!IsWriteable(from_path))
  {
    FormatMessage("Unmoveable file*\"{}\"*{}", from_path.c_str(), std::strerror(errno));
    return result;
  }


  if( dest_dir_entry )
  {
    /* Ziel befindet sich im Sub-Tree */
    /*--------------------------------*/

    GetFileEntry(dest_dir_entry, to_file, &dest_file_entry);

    if( dest_file_entry )
    {
      /* Datei existiert */
      /*-----------------*/

      if( confirm )
      {
  term = InputChoise("file exist; overwrite (Y/N) ? ", "YN\033");

        if( term != 'Y' ) {
    result = (term == 'N');  /* Abort on escape */
    return result;
  }
      }

      dest_file_entry->Delete();
    }
  }
  /* access benutzen */
  /*-----------------*/
  else if (std::filesystem::exists(to_path))
  {
    /* Datei existiert */
    /*-----------------*/

    if( confirm )
    {
      term = InputChoise("file exist; overwrite (Y/N) ? ", "YN\033");
      if (term != 'Y')
      {
        result = (term == 'N');  /* Abort on escape */
        return result;
      }
    }

    if (unlink(to_path.c_str()))
    {
      FormatMessage("Can't unlink*\"{}\"*{}", to_path.c_str(), std::strerror(errno));
      return result;
    }
  }


  if (Move(to_path, from_path))
  {
    /* File wurde bewegt */
    /*-------------------*/

    /* Original aus Baum austragen */
    /*-----------------------------*/

    fe_ptr->Remove();


    if( dest_dir_entry )
    {
      const auto meta = MetaOrAbort(to_path);

      file_size = meta.size;

      dest_dir_entry->total_bytes += file_size;
      dest_dir_entry->total_files++;
      statistic.disk_total_bytes += file_size;
      statistic.disk_total_files++;
      dest_dir_entry->matching_bytes += file_size;
      dest_dir_entry->matching_files++;
      statistic.disk_matching_bytes += file_size;
      statistic.disk_matching_files++;

      /* File eintragen */
      /*----------------*/
      auto fen_ptr = std::make_shared<FileEntry>();
      fen_ptr->name = to_file;
      fen_ptr->meta = meta;
      fen_ptr->dir_entry   = WeakDirEntry(dest_dir_entry);
      fen_ptr->matching    = Match(fen_ptr->name);
      dest_dir_entry->files.insert(dest_dir_entry->files.begin(), fen_ptr);
      *new_fe_ptr          = fen_ptr.get();
    }

    GetAvailBytes(statistic.disk_space);

    result = true;
  }

  return result;
}





static bool Move(const std::string& to_path, const std::string& from_path)
{
  if (!to_path.compare(from_path))
  {
    Message("Can't move file into itself");

    return false;
  }

  if (link(from_path.c_str(), to_path.c_str()))
  {
    FormatMessage("Can't link \"{}\"*to \"{}\"*{}", from_path.c_str(), to_path.c_str(), std::strerror(errno));

    return false;
  }

  if (unlink(from_path.c_str()))
  {
    FormatMessage("Can't unlink*\"{}\"*{}", from_path.c_str(), std::strerror(errno));

    return false;
  }

  return true;
}
