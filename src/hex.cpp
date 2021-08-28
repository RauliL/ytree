#include "ytree.h"

static int ViewHexFile(const std::string& file_path);
static int ViewHexArchiveFile(const std::string& file_path);

int ViewHex(const std::string& file_path)
{
  switch (mode)
  {
    case DISK_MODE:
    case USER_MODE:
      return ViewHexFile(file_path);

    case TAPE_MODE:
    case RPM_FILE_MODE:
    case TAR_FILE_MODE:
    case ZOO_FILE_MODE:
    case ZIP_FILE_MODE:
    case LHA_FILE_MODE:
    case ARC_FILE_MODE:
      return ViewHexArchiveFile(file_path);

    default:
      beep();
  }

  return -1;
}

static int ViewHexFile(const std::string& file_path)
{
  char* command_line;
  int result = -1;

  if (access(file_path.c_str(), R_OK))
  {
    std::snprintf(
      message,
      MESSAGE_LENGTH,
      "HexView not possible!*\"%s\"*%s",
      file_path.c_str(),
      std::strerror(errno)
    );
    Message(message);

    return -1;
  }

  InternalView(file_path);

  return 0;

  command_line = static_cast<char*>(std::malloc(COMMAND_LINE_LENGTH + 1));
  if (!command_line)
  {
    ERROR_MSG("Malloc failed*ABORT");
    std::exit(EXIT_FAILURE);
  }

  const auto compress_method = GetFileMethod(file_path);

  if (compress_method && *compress_method == CompressMethod::FREEZE_COMPRESS)
  {
    std::snprintf(
      command_line,
      COMMAND_LINE_LENGTH,
		  "%s < '%s' %s | %s | %s",
		  MELT,
		  file_path.c_str(),
		  ERR_TO_STDOUT,
		  HEXDUMP,
		  PAGER
		);
  }
  else if (compress_method && *compress_method == CompressMethod::COMPRESS_COMPRESS)
  {
    std::snprintf(
      command_line,
      COMMAND_LINE_LENGTH,
		  "%s < '%s' %s | %s | %s",
		  UNCOMPRESS,
		  file_path.c_str(),
		  ERR_TO_STDOUT,
		  HEXDUMP,
		  PAGER
		);
  }
  else if (compress_method && *compress_method == CompressMethod::GZIP_COMPRESS)
  {
    std::snprintf(
      command_line,
      COMMAND_LINE_LENGTH,
		  "%s < '%s' %s | %s | %s",
		  GNUUNZIP,
		  file_path.c_str(),
		  ERR_TO_STDOUT,
		  HEXDUMP,
		  PAGER
		);
  } else {
    std::snprintf(
      command_line,
      COMMAND_LINE_LENGTH,
		  "%s '%s' | %s",
		  HEXDUMP,
		  file_path.c_str(),
		  PAGER
		);
  }

  if ((result = SilentSystemCall(command_line)))
  {
    std::snprintf(
      message,
      MESSAGE_LENGTH,
      "can't execute*%s",
      command_line
    );
    Message(message);
  }

  std::free(command_line);

  return result;
}

static int ViewHexArchiveFile(const std::string& file_path)
{
  char* command_line = static_cast<char*>(std::malloc(COMMAND_LINE_LENGTH));
  char* archive;
  char buffer[80];
  int result = -1;

  if (!command_line)
  {
    ERROR_MSG("Malloc failed*ABORT");
    std::exit(EXIT_FAILURE);
  }

  std::snprintf(
    buffer,
    sizeof(buffer),
    "| %s | %s",
    HEXDUMP,
    PAGER
  );

  archive = mode == TAPE_MODE ? statistic.tape_name : statistic.login_path;

  MakeExtractCommandLine(
    command_line,
    COMMAND_LINE_LENGTH,
    archive,
		file_path,
    buffer
  );

  if ((result = SilentSystemCall(command_line)))
  {
    std::snprintf(
      message,
      MESSAGE_LENGTH,
      "can't execute*%s",
      command_line
    );
    Message(message);
  }

  std::free(command_line);

  return result;
}
