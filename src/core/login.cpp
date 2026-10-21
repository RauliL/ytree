#include <functional>

#include <sys/wait.h>
#include <unistd.h>

#include "core/api.hpp"
#include "core/profile.hpp"
#include "core/ui.hpp"

using read_tree_callback = std::function<
  bool(const std::shared_ptr<DirEntry>&, FILE*)
>;

bool ReadTreeFrom7ZIP(const std::shared_ptr<DirEntry>&, FILE*);
bool ReadTreeFromARC(const std::shared_ptr<DirEntry>&, FILE*);
bool ReadTreeFromLHA(const std::shared_ptr<DirEntry>&, FILE*);
bool ReadTreeFromRAR(const std::shared_ptr<DirEntry>&, FILE*);
bool ReadTreeFromRPM(const std::shared_ptr<DirEntry>&, FILE*);
bool ReadTreeFromTAR(const std::shared_ptr<DirEntry>&, FILE*);
bool ReadTreeFromZIP(const std::shared_ptr<DirEntry>&, FILE*);
bool ReadTreeFromZOO(const std::shared_ptr<DirEntry>&, FILE*);

static const std::unordered_map<Mode, read_tree_callback> read_tree_mapping({
  { Mode::ARC_FILE_MODE, ReadTreeFromARC },
  { Mode::LHA_FILE_MODE, ReadTreeFromLHA },
  { Mode::RAR_FILE_MODE, ReadTreeFromRAR },
  { Mode::RPM_FILE_MODE, ReadTreeFromRPM },
  { Mode::SEVENZIP_FILE_MODE, ReadTreeFrom7ZIP },
  { Mode::ZIP_FILE_MODE, ReadTreeFromZIP },
  { Mode::ZOO_FILE_MODE, ReadTreeFromZOO },
});

bool LoginDisk(const std::string& path)
{
  std::string command_line;
  std::optional<CompressMethod> file_method;
  int    pid;
  int    p[2];
  int    depth, l = 0;
  FILE   *f;
  int    status;
  int    result = 0;

  if( mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
  {
    /* Status retten */
    /*---------------*/
    disk_statistic = statistic;
  }

  if (!disk_statistic.login_path.empty()) {
    if( path == disk_statistic.login_path )
    {
      /* Tree is in memory! Use it! */
      /*----------------------------*/

      /* The previous tree (if different) is released by the assignment below. */

      if (IsUserActionDefined())
      {
        mode = Mode::USER_MODE;
      }
      else
      {
        mode = Mode::DISK_MODE;
      }
      statistic = disk_statistic;
      SetFileSpec(statistic.file_spec);

      return true; // Tree already in memory.
    }
  }

  std::error_code ec;
  auto root_meta = EntryMeta::FromFilesystem(path, ec);
  if (!root_meta)
  {
    // Stat failed.
    FormatMessage("Can't access*\"{}\"*{}", path, ec.message());

    return false;
  }

  /* Assigning a fresh Statistic releases the previous tree (shared_ptr). */
  statistic = {};

  statistic.tree = std::make_shared<DirEntry>();

  statistic.path = path;
  statistic.login_path = path;
  statistic.file_spec = "*";
  statistic.tape_name = DEFAULT_TAPEDEV;
  statistic.kind_of_sort = { SortKey::Name, SortOrder::Ascending };
  statistic.tree->meta = *root_meta;


  if( !root_meta->IsDir() )
  {
    /* No Directory ==> TAR_FILE/RPM/ZOO/ZIP/LHA/ARC_FILE */
    /*----------------------------------------------------*/
    file_method = GetFileMethod(statistic.login_path);
    l = static_cast<int>(statistic.login_path.size());
    if (!file_method)
    {
      mode = Mode::TAR_FILE_MODE;
    } else {
      switch (*file_method)
      {
        case CompressMethod::ZOO_COMPRESS:
          mode = Mode::ZOO_FILE_MODE;
          break;

        case CompressMethod::ARC_COMPRESS:
          mode = Mode::ARC_FILE_MODE;
          break;

        case CompressMethod::LHA_COMPRESS:
          mode = Mode::LHA_FILE_MODE;
          break;

        case CompressMethod::ZIP_COMPRESS:
          mode = Mode::ZIP_FILE_MODE;
          break;

        case CompressMethod::RPM_COMPRESS:
          mode = Mode::RPM_FILE_MODE;
          break;

        case CompressMethod::RAR_COMPRESS:
          mode = Mode::RAR_FILE_MODE;
          break;

        case CompressMethod::SEVENZIP_COMPRESS:
          mode = Mode::SEVENZIP_FILE_MODE;
          break;

        case CompressMethod::TAPE_DIR_NO_COMPRESS:
        case CompressMethod::TAPE_DIR_COMPRESS_COMPRESS:
        case CompressMethod::TAPE_DIR_FREEZE_COMPRESS:
        case CompressMethod::TAPE_DIR_GZIP_COMPRESS:
        case CompressMethod::TAPE_DIR_BZIP_COMPRESS:
        case CompressMethod::TAPE_DIR_XZ_COMPRESS:
        case CompressMethod::TAPE_DIR_ZSTD_COMPRESS:
          mode = Mode::TAPE_MODE;
          break;

        default:
          mode = Mode::TAR_FILE_MODE;
          break;
      }
    }
  }
  else if (IsUserActionDefined())
  {
    mode = Mode::USER_MODE;
  }
  else
  {
    mode = Mode::DISK_MODE;
  }


  GetDiskParameter(
    path,
    &statistic.disk_name,
    statistic.disk_space,
    &statistic.disk_capacity
  );

  ui::RefreshChrome();


  if( mode == Mode::TAPE_MODE )
  {
    /* zugehoeriges tape-device ermitteln */
    /*------------------------------------*/

    if (const auto tape = ui::Prompt("Tape-Device:", statistic.tape_name))
    {
      statistic.tape_name = *tape;
    } else {
      return false;
    }
  }


  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    statistic.tree->name = path;

    if (pipe(p))
    {
      YTreeError("pipe() failed");

      return false;
    }

    if (!file_method)
    {
      /* NO_COMPRESS */
      /*-------------*/

      /* gtar tvf - < TAR_FILE */
      /*-----------------------*/
      command_line = std::format(
        "{} < '{}'",
        GetProfileValueOrEmpty("TARLIST"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::ZOO_COMPRESS)
    {
      /* zoo vom ZOO_FILE */
      /*------------------*/
      command_line = std::format(
        "{} '{}'",
        GetProfileValueOrEmpty("ZOOLIST"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::RPM_COMPRESS)
    {
      /* rpm vom RPM_FILE */
      /*------------------*/
      command_line = std::format(
        "{} '{}'",
        GetProfileValueOrEmpty("RPMLIST"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::LHA_COMPRESS)
    {
      /* LHA_FILE */
      /*----------*/
      command_line = std::format(
        "{} '{}'",
        GetProfileValueOrEmpty("LHALIST"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::ZIP_COMPRESS)
    {
      /* ZIP_FILE */
      /*----------*/
      command_line = std::format(
        "{} '{}'",
        GetProfileValueOrEmpty("ZIPLIST"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::ARC_COMPRESS)
    {
      /* ARC_FILE */
      /*----------*/
      command_line = std::format(
        "{} '{}'",
        GetProfileValueOrEmpty("ARCLIST"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::RAR_COMPRESS)
    {
      /* RAR_FILE */
      /*----------*/
      command_line = std::format(
        "{} '{}'",
        GetProfileValueOrEmpty("RARLIST"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::SEVENZIP_COMPRESS)
    {
      /* 7ZIP_FILE */
      /*-----------*/
      command_line = std::format(
        "{} '{}'",
        GetProfileValueOrEmpty("SEVENZIPLIST"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::FREEZE_COMPRESS)
    {
      /* melt < TAR_FILE | gtar tvf - */
      /*------------------------------*/
      command_line = std::format(
        "{} < '{}' {} | {}",
        GetProfileValueOrEmpty("MELT"),
        statistic.login_path,
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::MULTIPLE_FREEZE_COMPRESS)
    {
      const auto cat_file = statistic.login_path.substr(0, l - 2) + "*";

      /* cat TAR_FILE | melt | gtar tvf - */
      /*----------------------------------*/
      command_line = std::format(
        "{} '{}' {} | {} | {}",
        GetProfileValueOrEmpty("CAT"),
        cat_file,
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("MELT"),
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::COMPRESS_COMPRESS)
    {
      /* uncompress < TAR_FILE | gtar tvf - */
      /*------------------------------------*/
      command_line = std::format(
        "{} < '{}' {} | {}",
        GetProfileValueOrEmpty("UNCOMPRESS"),
        statistic.login_path,
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::MULTIPLE_COMPRESS_COMPRESS)
    {
      const auto cat_file = statistic.login_path.substr(0, l - 2) + "*";

      /* cat TAR_FILE.X* | uncompress | gtar tvf - */
      /*-------------------------------------------*/
      command_line = std::format(
        "{} {} | {} {} | {}",
        GetProfileValueOrEmpty("CAT"),
        cat_file,
        GetProfileValueOrEmpty("UNCOMPRESS"),
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::GZIP_COMPRESS)
    {
      /* gunzip < TAR_FILE | gtar tvf - */
      /*--------------------------------*/
      command_line = std::format(
        "{} < '{}' {} | {}",
        GetProfileValueOrEmpty("GNUUNZIP"),
        statistic.login_path,
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::MULTIPLE_GZIP_COMPRESS)
    {
      const auto cat_file = statistic.login_path.substr(0, l - 2) + "*";

      /* cat TAR_FILE.X* | gunzip | gtar tvf - */
      /*---------------------------------------*/
      command_line = std::format(
        "{} {} | {} {} | {}",
        GetProfileValueOrEmpty("CAT"),
        cat_file,
        GetProfileValueOrEmpty("GNUUNZIP"),
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::BZIP_COMPRESS)
    {
      /* bunzip2 < TAR_FILE | gtar tvf - */
      /*---------------------------------*/
      command_line = std::format(
        "{} < '{}' {} | {}",
        GetProfileValueOrEmpty("BUNZIP"),
        statistic.login_path,
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::XZ_COMPRESS)
    {
      /* xz -dc < TAR_FILE | gtar tvf - */
      /*--------------------------------*/
      command_line = std::format(
        "{} < '{}' {} | {}",
        GetProfileValueOrEmpty("UNXZ"),
        statistic.login_path,
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::ZSTD_COMPRESS)
    {
      /* zstd -dc < TAR_FILE | gtar tvf - */
      /*----------------------------------*/
      command_line = std::format(
        "{} < '{}' {} | {}",
        GetProfileValueOrEmpty("UNZSTD"),
        statistic.login_path,
        ERR_TO_STDOUT,
        GetProfileValueOrEmpty("TARLIST")
      );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_FREEZE_COMPRESS)
    {
      /* melt < TAR_FILE */
      /*-----------------*/
      command_line = std::format(
        "{} < '{}'",
        GetProfileValueOrEmpty("MELT"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_COMPRESS_COMPRESS)
    {
      /* uncompress < TAR_FILE */
      /*-----------------------*/
      command_line = std::format(
        "{} < '{}'",
        GetProfileValueOrEmpty("UNCOMPRESS"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_GZIP_COMPRESS)
    {
      /* gunzip < TAR_FILE */
      /*-------------------*/
      command_line = std::format(
        "{} < '{}'",
        GetProfileValueOrEmpty("GNUUNZIP"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_BZIP_COMPRESS)
    {
      /* bunzip2 < TAR_FILE */
      /*--------------------*/
      command_line = std::format(
        "{} < '{}'",
        GetProfileValueOrEmpty("BUNZIP"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_XZ_COMPRESS)
    {
      /* xz -dc < TAR_FILE */
      /*-------------------*/
      command_line = std::format(
        "{} < '{}'",
        GetProfileValueOrEmpty("UNXZ"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_ZSTD_COMPRESS)
    {
      /* zstd -dc < TAR_FILE */
      /*---------------------*/
      command_line = std::format(
        "{} < '{}'",
        GetProfileValueOrEmpty("UNZSTD"),
        statistic.login_path
      );
    }
    else if (*file_method == CompressMethod::TAPE_DIR_NO_COMPRESS)
    {
      /* cat < TAR_FILE */
      /*----------------*/
      command_line = std::format(
        "{} < '{}'",
        GetProfileValueOrEmpty("CAT"),
        statistic.login_path
      );
    } else {
      FormatError("unknown file method {}", static_cast<int>(*file_method));
      close(p[0]);
      close(p[1]);

      return false;
    }

    command_line += " 2> /dev/null";

#ifdef DEBUG
    std::fprintf(stderr, "system( \"%s\" )\n", command_line.c_str());
#endif

    pid = fork();

    if (pid == -1)
    {
      YTreeError("fork() failed");
      close(p[0]);
      close(p[1]);

      return false;
    }
    else if( pid == 0 )
    {
      // Sohn.

      close(p[0]);
      close(1);
      dup(p[1]);
      close(p[1]);

      if (result == 0 && !SilentSystemCall(command_line))
      {
        result = 1;
        // std::fprintf(stderr, "system(%s)*failed\n", command_line);
      }
      std::exit(result);
    } else {
      // Vater.
      read_tree_callback callback = ReadTreeFromTAR;

      close(p[1]);
      status = 0;

      if (!(f = fdopen(p[0], "r")))
      {
        YTreeError("fdopen() failed");

        return false;
      }

      if (
        const auto it = read_tree_mapping.find(mode);
        it != std::end(read_tree_mapping)
      )
      {
        callback = it->second;
      }
      if (!callback(statistic.tree, f))
      {
        YTreeError("Reading tree from archive failed");
        std::fclose(f);
        wait(&status);

        /* Restore the previous disk tree if we still have one. */
        if (!disk_statistic.login_path.empty())
        {
          statistic = disk_statistic;
          mode = IsUserActionDefined() ? Mode::USER_MODE : Mode::DISK_MODE;
        }

        return false;
      }
      wait(&status);
      if (status)
      {
        FormatMessage("ReadTarFile() failed*can't execute*{}", command_line);
      }
      std::fclose(f);
    }
  }
  else
  {
    if (!disk_statistic.login_path.empty())
    {
      /* Alten Baum loeschen */
      /*---------------------*/
      disk_statistic.login_path.clear();
      disk_statistic.tree.reset();
    }

    statistic.tree->name = path;

    depth = GetDoubleProfileValue("TREEDEPTH");
    if (!ReadTree(statistic.tree, path, depth))
    {
      YTreeError("ReadTree() failed");

      return false;
    }
    disk_statistic = statistic;
  }

  SetFileSpec(statistic.file_spec);
/*  SetKindOfSort( statistic.kind_of_sort ); */

  return true;
}





std::optional<std::string> GetNewLoginPath(const std::string& initial)
{
  std::string aux = initial;

  if (mode == Mode::LL_FILE_MODE && !initial.empty() && initial.front() == '<')
  {
    if (!aux.empty())
    {
      aux.erase(0, 1);
    }
    if (!aux.empty() && aux.back() == '>')
    {
      aux.pop_back();
    }
  }

  if (const auto entered = ui::Prompt("NEW LOGIN-PATH:", aux))
  {
    return NormPath(*entered);
  }

  return std::nullopt;
}
