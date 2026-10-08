/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/login.c,v 1.23 2014/12/26 09:53:11 werner Exp $
 *
 * Dateibaum lesen
 *
 ***************************************************************************/


#include "ytree.h"
/* #include <sys/wait.h> */  /* maybe wait.h is available */




/* Login Disk: true on success, false on failure. */


bool LoginDisk(const std::string& path)
{
  struct stat stat_struct;
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
      return true;   /* Tree already in memory */
    }
  }


  if( STAT_(path.c_str(), &stat_struct) )
  {
    /* Stat failed */
    /*-------------*/
    FormatMessage("Can't access*\"{}\"*{}", path, std::strerror(errno));

    return false;
  }


  /* Assigning a fresh Statistic releases the previous tree (shared_ptr). */
  statistic = {};

  statistic.tree = std::make_shared<DirEntry>();

  statistic.path = path;
  statistic.login_path = path;
  statistic.file_spec = DEFAULT_FILE_SPEC;
  statistic.tape_name = DEFAULT_TAPEDEV;
  statistic.kind_of_sort = { SortKey::Name, SortOrder::Ascending };
  statistic.tree->stat_struct = stat_struct;


  if( !S_ISDIR(stat_struct.st_mode) )
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
    &statistic.disk_space,
    &statistic.disk_capacity
  );

  RefreshWindow(stdscr);
  RefreshWindow(dir_window);
  DisplayMenu();
  doupdate();


  if( mode == Mode::TAPE_MODE )
  {
    /* zugehoeriges tape-device ermitteln */
    /*------------------------------------*/

    if (!GetTapeDeviceName())
    {
      return false;
    }
  }


  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    statistic.tree->name = path;

    if (pipe(p))
    {
      Error("pipe() failed");

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

    command_line += ERR_TO_NULL;

#ifdef DEBUG
  std::fprintf(stderr, "system( \"%s\" )\n", command_line.c_str());
#endif

    pid = fork();

    if (pid == -1)
    {
      Error("fork() failed");
      close(p[0]);
      close(p[1]);

      return false;
    }
    else if( pid == 0 )
    {
      /* Sohn */
      /*------*/

      close(p[0]);
      close(1);
      if(dup(p[1]) == -1)
      {
  /* fprintf(stderr, "dup failed\n" ); */
      }
      close(p[1]);

      if( result == 0 && SilentSystemCallEx(command_line, false) )
      {
        result = 1;
  /* fprintf(stderr, "system(%s)*failed\n", command_line ); */
      }
      std::exit(result);
    }
    else
    {
      /* Vater */
      /*-------*/

      close(p[1]);
      status = 0;

      if (!(f = fdopen(p[0], "r")))
      {
        Error("fdopen() failed");

        return false;
      }

      if( mode == Mode::ZOO_FILE_MODE )
      {
  if( ReadTreeFromZOO(statistic.tree, f) )
        {
    Error("ReadTreeFromZOO() failed");
          std::fclose(f);
    wait(&status);
          return false;
  }
      }
      else if( mode == Mode::RPM_FILE_MODE )
      {
  if( ReadTreeFromRPM(statistic.tree, f) )
        {
    Error("ReadTreeFromRPM() failed");
          std::fclose(f);
    wait(&status);
          return false;
  }
      }
      else if( mode == Mode::LHA_FILE_MODE )
      {
  if( ReadTreeFromLHA(statistic.tree, f) )
        {
    Error("ReadTreeFromLHA() failed");
          std::fclose(f);
    wait(&status);
          return false;
  }
      }
      else if( mode == Mode::ZIP_FILE_MODE )
      {
  if( ReadTreeFromZIP(statistic.tree, f) )
        {
    Error("ReadTreeFromZIP() failed");
          std::fclose(f);
    wait(&status);
          return false;
  }
      }
      else if( mode == Mode::ARC_FILE_MODE )
      {
  if( ReadTreeFromARC(statistic.tree, f) )
        {
    Error("ReadTreeFromARC() failed");
          std::fclose(f);
    wait(&status);
          return false;
  }
      }
      else if( mode == Mode::RAR_FILE_MODE )
      {
  if( ReadTreeFromRAR(statistic.tree, f) )
        {
    Error("ReadTreeFromRAR() failed");
          std::fclose(f);
    wait(&status);
          return false;
  }
      }
      else if (mode == Mode::SEVENZIP_FILE_MODE)
      {
        if (ReadTreeFrom7ZIP(statistic.tree, f))
        {
          Error("ReadTreeFrom7ZIP() failed");
          std::fclose(f);
          wait(&status);
          return false;
        }
      }
      else
      {
        if( ReadTreeFromTAR(statistic.tree, f) )
        {
          Error("ReadTreeFromTAR() failed");
          std::fclose(f);
    wait(&status);
          return false;
        }
      }
      wait(&status);
      if(status)
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
    if (ReadTree(statistic.tree, path, depth))
    {
      Error("ReadTree() failed");

      return false;
    }
    disk_statistic = statistic;
  }

  SetFileSpec(statistic.file_spec);
/*  SetKindOfSort( statistic.kind_of_sort ); */

  return true;
}





bool GetNewLoginPath(std::string& path)
{
  std::string aux = path;

  ClearHelp();

  MvAddStr(LINES - 2, 1, "NEW LOGIN-PATH:");

  if (mode == Mode::LL_FILE_MODE && !path.empty() && path.front() == '<')
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

  if (InputString(aux, LINES - 2, 17, 0, COLS - 24) == CR)
  {
    path = NormPath(aux);
    return true;
  }

  return false;
}


