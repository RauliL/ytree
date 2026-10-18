#include "./ytree.hpp"

static bool RenameDirEntry(
  const std::filesystem::path&,
  const std::filesystem::path&
);
static bool RenameFileEntry(
  const std::filesystem::path&,
  const std::filesystem::path&
);

int RenameDirectory(DirEntry* de_ptr, const std::string& new_name)
{
  const auto from_path = GetPath(de_ptr);
  const std::filesystem::path from_fs_path(from_path);
  std::string to_path;
  int result = -1;

  if (!from_fs_path.has_filename() ||
      from_fs_path == std::filesystem::path(
        std::string_view(&std::filesystem::path::preferred_separator, 1)))
  {
    Message("Can't rename ROOT");
  } else {
    auto to_path = (from_fs_path.parent_path() / new_name).string();

    if (IsWriteable(from_path))
    {
      if (RenameDirEntry(to_path, from_path))
      {
        // The entry is owned by shared_ptr and only holds weak
        // back-references, so it can be renamed in place: parent, children,
        // files, statistic.tree and disk_statistic.tree all stay valid.
        de_ptr->name = new_name;
        de_ptr->SetMeta(MetaOrAbort(to_path));

        result = 0;
      }
    } else {
      FormatMessage(
        "Rename not possible!*\"{}\"*{}",
        from_path.string(),
        std::strerror(errno)
      );
    }
  }

  move(LINES - 2, 1);
  clrtoeol();

  return result;
}

int RenameFile(
  FileEntry* fe_ptr,
  const std::string& new_name,
  FileEntry** new_fe_ptr
)
{
  const auto de_ptr = fe_ptr->Dir();
  const auto from_path = fe_ptr->GetPath();
  const auto to_path =
    (std::filesystem::path(GetPath(de_ptr.get())) / new_name).string();
  int result = -1;

  *new_fe_ptr = fe_ptr;

  if (!IsWriteable(from_path))
  {
    FormatMessage(
      "Rename not possible!*\"{}\"*{}",
      from_path.string(),
      std::strerror(errno)
    );

    return -1;
  }

  if (RenameFileEntry(to_path, from_path))
  {
    // Rename in place; see RenameDirectory().
    fe_ptr->name = new_name;
    fe_ptr->SetMeta(MetaOrAbort(to_path));

    result = 0;

  }

  move(LINES - 2, 1);
  clrtoeol();

  return result;
}

std::optional<std::string> GetRenameParameter(const std::string* old_name)
{
  int l;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return std::nullopt;
  }

  ClearHelp();

  if (!old_name)
  {
    MvAddStr(LINES - 2, 1, "RENAME TAGGED FILES TO:");
    l = 25;
  } else {
    MvAddStr(LINES - 2, 1, "RENAME TO:");
    l = 13;
  }

  const auto edited_name = InputString(
    old_name ? *old_name : "*",
    LINES - 2,
    l,
    0,
    COLS - l - 1
  );
  if (!edited_name)
  {
    return std::nullopt;
  }

  if (edited_name->empty())
  {
    return std::nullopt;
  }

  if (old_name && *old_name == *edited_name)
  {
    Message("Can't rename: New name same as old name.");

    return std::nullopt;
  }

  if (edited_name->find(std::filesystem::path::preferred_separator) != std::string::npos)
  {
    Message("Invalid new name:*No slashes when renaming!");

    return std::nullopt;
  }

  return edited_name;
}

static bool RenameDirEntry(
  const std::filesystem::path& to_path,
  const std::filesystem::path& from_path
)
{
  if (!to_path.compare(from_path))
  {
    Message("Can't rename directory:*New Name == Old Name");

    return false;
  }

  if (std::filesystem::exists(to_path))
  {
    Message("Can't rename directory:*Destination object already exist!");

    return false;
  }

  std::error_code ec;
  std::filesystem::rename(from_path, to_path, ec);
  if (ec)
  {
    FormatMessage("Can't rename \"{}\"*to \"{}\"*{}", from_path.c_str(), to_path.c_str(), ec.message().c_str());

    return false;
  }

  return true;
}

static bool RenameFileEntry(
  const std::filesystem::path& to_path,
  const std::filesystem::path& from_path)
{
  if (!to_path.compare(from_path))
  {
    Message("Can't rename!*New Name == Old Name");

    return false;
  }

  if (std::filesystem::exists(to_path))
  {
    Message("Can't rename!*Destination object already exist!");

    return false;
  }

  std::error_code ec;
  std::filesystem::rename(from_path, to_path, ec);
  if (ec)
  {
    FormatMessage(
      "Can't rename \"{}\"*to \"{}\"*{}",
      from_path.string(),
      to_path.string(),
      ec.message()
    );

    return false;
  }

  return true;
}
