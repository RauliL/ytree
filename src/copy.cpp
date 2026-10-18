#include "./ytree.hpp"

static int Copy(const std::string& to_path, const std::string& from_path);
static int CopyArchiveFile(const std::string& to_path, const std::string& from_path);

int CopyFile(
  Statistic* statistic_ptr,
  FileEntry* fe_ptr,
  bool confirm,
  const std::string& to_file,
  DirEntry* dest_dir_entry,
  const std::string& to_dir_path, /* absoluter Pfad */
  bool path_copy
)
{
  std::int64_t file_size;
  const auto from_path = fe_ptr->GetRealPath();
  const auto from_dir = GetPath(fe_ptr->Dir().get());
  std::filesystem::path to_fs_path;
  std::string to_path;
  FileEntry   *dest_file_entry;
  int         term;
  int         result;
  int       refresh_dirwindow = false;


  result = -1;

  const ScopeGuard clear_prompt_lines([] { ClearBottomPromptLines(); });

  const auto is_root =
    to_dir_path.size() == 1 &&
    to_dir_path.front() == std::filesystem::path::preferred_separator;

  if (!is_root)
  {
    /* not ROOT */
    /*----------*/

    to_fs_path = to_dir_path;
  }
  if (path_copy)
  {
    const auto path = std::filesystem::path(GetPath(fe_ptr->Dir().get()));

    /* Create destination folder (if neccessary) */
    /*-------------------------------------------*/
    if (to_fs_path.empty())
    {
      to_fs_path = path;
    }
    else
    {
      /* strcat-style append: avoid absolute-path replacement */
      to_fs_path /= path.is_absolute() ? path.relative_path() : path;
    }

    if (!to_fs_path.is_absolute())
    {
      to_fs_path = std::filesystem::path(from_dir) / to_fs_path;
    }

    to_path = to_fs_path.string();
    if (MakePath(statistic_ptr->tree, to_path, &dest_dir_entry))
    {
      FormatMessage("Can't create path*\"{}\"*{}", to_path.c_str(), std::strerror(errno));

      return result;
    }
  }
  {
    std::error_code ec;
    const auto dir_path = to_fs_path.empty()
      ? std::filesystem::path(
          std::string_view(&std::filesystem::path::preferred_separator, 1))
      : to_fs_path;

    if (!std::filesystem::is_directory(dir_path, ec) &&
        !std::filesystem::exists(dir_path, ec))
    {
      if ((term = InputChoise(
            "Directory does not exist; create (y/N) ? ",
            "YN\033"
)) == 'Y')
      {
        if (!to_fs_path.empty() && !to_fs_path.is_absolute())
        {
          to_fs_path = std::filesystem::path(from_dir) / to_fs_path;
        }
        to_path = to_fs_path.string();
        if (MakePath(statistic_ptr->tree, to_path, &dest_dir_entry))
        {
          FormatMessage("Can't create path*\"{}\"*{}", to_path.c_str(), std::strerror(errno));

          return result;
        }
        else
        {
          refresh_dirwindow = true;
        }
      }
      else
      {
        return result;
      }
    }
  }
  if (to_fs_path.empty())
  {
    to_fs_path = std::string_view(&std::filesystem::path::preferred_separator, 1);
  }
  to_fs_path /= to_file;
  to_path = to_fs_path.string();


#ifdef DEBUG
  std::fprintf(stderr, "Copy: \"%s\" --> \"%s\"\n", from_path.c_str(), to_path.c_str());
#endif /* DEBUG */

  if (to_path == from_path)
  {
    Message("Can't copy file into itself");

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

        if( term != 'Y' )
        {
    result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
          return result;
        }
      }

      DeleteFile(dest_file_entry);
    }
  }
  /* access benutzen */
  /*-----------------*/
  else if (std::filesystem::exists(to_path) && confirm)
  {
    /* Datei existiert */
    /*-----------------*/
    term = InputChoise("file exist; overwrite (Y/N) ? ", "YN\033");
    if (term != 'Y')
    {
      result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
      return result;
    }
  }


  if( !Copy(to_path, from_path) )
  {
    /* File wurde kopiert */
    /*--------------------*/

    if( chmod(to_path.c_str(), fe_ptr->meta.Mode()) == -1 )
    {
      FormatWarning("Can't chmod file*\"{}\"*to mode {}*IGNORED", to_path.c_str(), GetAttributes(fe_ptr->meta.Mode()));
    }

    if( dest_dir_entry )
    {
      const auto meta = MetaOrAbort(to_path);

      file_size = meta.size;

      dest_dir_entry->total_bytes += file_size;
      dest_dir_entry->total_files++;
      statistic_ptr->disk_total_bytes += file_size;
      statistic_ptr->disk_total_files++;
      dest_dir_entry->matching_bytes += file_size;
      dest_dir_entry->matching_files++;
      statistic_ptr->disk_matching_bytes += file_size;
      statistic_ptr->disk_matching_files++;

      /* File eintragen */
      /*----------------*/
      auto fen_ptr = std::make_shared<FileEntry>();
      fen_ptr->name = to_file;
      fen_ptr->SetMeta(meta);
      fen_ptr->dir_entry   = dest_dir_entry->weak_from_this();
      fen_ptr->matching    = Match(fen_ptr->name);
      dest_dir_entry->files.insert(dest_dir_entry->files.begin(), fen_ptr);
    }

    GetAvailBytes(&statistic_ptr->disk_space);

    result = 0;
  }

  if( refresh_dirwindow)
  {
    RefreshDirWindow();
  }

  return result;
}





static std::optional<AsToParameter> GetAsToParameter(
  const std::string& header,
  std::optional<std::string_view> from_file
)
{
  std::string to_file_input = from_file ? std::string(*from_file) : "*";

  ClearHelp();

  MvAddStr(LINES - 3, 1, header);
  MvAddStr(LINES - 2, 1, "AS   ");

  const auto to_file_value = InputString(to_file_input, LINES - 2, 6, 0, COLS - 6);
  if (!to_file_value)
  {
    ClearHelp();
    return std::nullopt;
  }

  MvAddStr(LINES - 1, 1, "TO   ");
  const auto to_dir_value = InputString({}, LINES - 1, 6, 0, COLS - 6);
  if (!to_dir_value)
  {
    ClearHelp();
    return std::nullopt;
  }

  return AsToParameter{ *to_file_value, *to_dir_value };
}

std::optional<AsToParameter> GetCopyParameter(
  std::optional<std::string_view> from_file,
  bool path_copy
)
{
  const std::string_view label = from_file ? *from_file : "TAGGED FILES";
  const auto header = path_copy
    ? std::format("PATHCOPY {}", label)
    : std::format("COPY {}", label);

  return GetAsToParameter(header, from_file);
}

std::optional<AsToParameter> GetMoveParameter(
  std::optional<std::string_view> from_file
)
{
  const std::string_view label = from_file ? *from_file : "TAGGED FILES";
  const auto header = std::format("MOVE {}", label);

  return GetAsToParameter(header, from_file);
}

static int Copy(const std::string& to_path, const std::string& from_path)
{
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    return CopyArchiveFile(to_path, from_path);
  }

#ifdef DEBUG
  std::fprintf(stderr, "Copy: \"%s\" --> \"%s\"\n", from_path.c_str(), to_path.c_str());
#endif /* DEBUG */

  if (!to_path.compare(from_path))
  {
    Message("Can't copy file into itself");

    return -1;
  }

  std::error_code ec;
  std::filesystem::copy_file(
    from_path,
    to_path,
    std::filesystem::copy_options::overwrite_existing,
    ec
);
  if (ec)
  {
    FormatMessage("Can't copy file*\"{}\"*to*\"{}\"*{}", from_path.c_str(), to_path.c_str(), ec.message().c_str());

    return -1;
  }

  return 0;
}

static int CopyArchiveFile(
  const std::string& to_path,
  const std::string& from_path
)
{
  const auto command_line = MakeExtractCommandLine(
    mode == Mode::TAPE_MODE ? statistic.tape_name : statistic.login_path,
    from_path,
    "> " + ShellQuote(to_path)
);
  const auto result = SilentSystemCall(command_line);

  if (result)
  {
    FormatWarning("Can't copy file*{}*to file*{}", from_path.c_str(), to_path.c_str());
  }

  return result;
}
