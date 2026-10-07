#include "ytree.h"

static int ViewHexFile(const std::filesystem::path& file_path);
static int ViewHexArchiveFile(const std::filesystem::path& file_path);

int ViewHex(const std::filesystem::path& file_path)
{
  switch (mode)
  {
    using enum Mode;
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

static int ViewHexFile(const std::filesystem::path& file_path)
{
  if (!IsReadable(file_path))
  {
    FormatMessage(
      "HexView not possible!*\"{}\"*{}",
      file_path.c_str(),
      std::strerror(errno)
    );

    return -1;
  }

  return InternalView(file_path);
}

static int ViewHexArchiveFile(const std::filesystem::path& file_path)
{
  const auto command_line = MakeExtractCommandLine(
    mode == Mode::TAPE_MODE ? statistic.tape_name : statistic.login_path,
		file_path,
    std::format("| {} | {}", HEXDUMP, PAGER)
  );
  const auto result = SilentSystemCall(command_line);

  if (result)
  {
    FormatMessage("can't execute*{}", command_line.c_str());
  }

  return result;
}
