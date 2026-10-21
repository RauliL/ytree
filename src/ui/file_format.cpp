#include "core/api.hpp"
#include "core/profile.hpp"
#include "ui/file_format.hpp"
#include "ui/ui.hpp"

static std::string MarkCell(const std::shared_ptr<FileEntry>& fe_ptr)
{
  if (!fe_ptr)
  {
    return {};
  }

  return std::format(
    "{}{}",
    fe_ptr->tagged ? TAGGED_SYMBOL : ' ',
    fe_ptr->meta.TypeSymbol()
  );
}

static inline std::string NameCell(const std::shared_ptr<FileEntry>& fe_ptr)
{
  return fe_ptr ? fe_ptr->name : std::string{};
}

static std::string LinkCell(const std::shared_ptr<FileEntry>& fe_ptr)
{
  if (!fe_ptr || !fe_ptr->meta.IsLink())
  {
    return {};
  }

  return fe_ptr->symlink_target;
}

static inline bool ShowLinkColumn()
{
  return GetFileMaxLinknameLen() > 0;
}

std::string FormatFileEntryLine(const std::shared_ptr<FileEntry>& fe_ptr)
{
  const char justify = GetFileReverseSort() ? '+' : '-';
  const char* sym_link_name = "";
  char type_of_file = ' ';

  if (fe_ptr && fe_ptr->meta.IsLink())
  {
    sym_link_name = fe_ptr->symlink_target.c_str();
  }
  if (fe_ptr)
  {
    type_of_file = fe_ptr->meta.TypeSymbol();
  }

  const auto fitted_name = fe_ptr
    ? FitVisualWidth(fe_ptr->name, GetFileMaxFilenameLen(), justify == '-')
    : std::string{};
  const auto fitted_link = FitVisualWidth(sym_link_name, GetFileMaxLinknameLen(), true);

  switch (GetFileViewMode())
  {
    case ViewMode::MODE_1:
      if (fe_ptr)
      {
        const auto attributes = GetAttributes(fe_ptr->meta.Mode());
        const auto modify_time = FormatMetaTime(fe_ptr->meta.mtime);
        const auto nlink = FormatMetaNlink(fe_ptr->meta);

        if (fe_ptr->meta.IsLink())
        {
          return std::format(
            "{}{}{} {:>10} {:>3} {:11} {:>12} -> {}",
            fe_ptr->tagged ? TAGGED_SYMBOL : ' ',
            type_of_file,
            fitted_name,
            attributes,
            nlink,
            static_cast<std::int64_t>(fe_ptr->meta.size),
            modify_time,
            fitted_link
          );
        }
        return std::format(
          "{}{}{} {:>10} {:>3} {:11} {:>12}",
          fe_ptr->tagged ? TAGGED_SYMBOL : ' ',
          type_of_file,
          fitted_name,
          attributes,
          nlink,
          static_cast<std::int64_t>(fe_ptr->meta.size),
          modify_time
        );
      }
      return std::format("{:<{}}", "", GetFileMaxFilenameLen() + 42);

    case ViewMode::MODE_2:
      if (fe_ptr)
      {
        const auto owner = FormatMetaOwner(fe_ptr->meta);
        const auto group = FormatMetaGroup(fe_ptr->meta);
        const auto inode = FormatMetaInode(fe_ptr->meta);

        if (fe_ptr->meta.IsLink())
        {
          return std::format(
            "{}{}{} {:>10} {:<12} {:<12} -> {}",
            fe_ptr->tagged ? TAGGED_SYMBOL : ' ',
            type_of_file,
            fitted_name,
            inode,
            owner,
            group,
            fitted_link
          );
        }
        return std::format(
          "{}{}{} {:>10} {:<12} {:<12}",
          fe_ptr->tagged ? TAGGED_SYMBOL : ' ',
          type_of_file,
          fitted_name,
          inode,
          owner,
          group
        );
      }
      return std::format("{:<{}}", "", GetFileMaxFilenameLen() + 38);

    case ViewMode::MODE_3:
      if (fe_ptr)
      {
        return std::format(
          "{}{}{}",
          fe_ptr->tagged ? TAGGED_SYMBOL : ' ',
          type_of_file,
          fitted_name
        );
      }
      return std::format("{:<{}}", "", GetFileMaxFilenameLen() + 2);

    case ViewMode::MODE_4:
      if (fe_ptr)
      {
        const auto change_time = FormatMetaTime(fe_ptr->meta.ctime);
        const auto access_time = FormatMetaTime(fe_ptr->meta.atime);

        if (fe_ptr->meta.IsLink())
        {
          return std::format(
            "{}{}{} Chg: {:>12}  Acc: {:>12} -> {}",
            fe_ptr->tagged ? TAGGED_SYMBOL : ' ',
            type_of_file,
            fitted_name,
            change_time,
            access_time,
            fitted_link
          );
        }
        return std::format(
          "{}{}{} Chg: {:>12}  Acc: {:>12}",
          fe_ptr->tagged ? TAGGED_SYMBOL : ' ',
          type_of_file,
          fitted_name,
          change_time,
          access_time
        );
      }
      return std::format("{:<{}}", "", GetFileMaxFilenameLen() + 39);

    case ViewMode::MODE_5:
      if (fe_ptr)
      {
        return BuildUserFileEntry(
          fe_ptr,
          GetFileMaxFilenameLen(),
          GetFileMaxLinknameLen(),
          GetProfileValueOrEmpty("USERVIEW"),
          200
        );
      }
      return std::format("{:<{}}", "", GetFileMaxUserviewLen());
  }

  return {};
}

int FileEntryColumnStride()
{
  switch (GetFileViewMode())
  {
    case ViewMode::MODE_1:
      if (GetFileMaxLinknameLen())
      {
        return static_cast<int>(GetFileMaxFilenameLen() + GetFileMaxLinknameLen() + 47);
      }
      return static_cast<int>(GetFileMaxFilenameLen() + 43);

    case ViewMode::MODE_2:
      if (GetFileMaxLinknameLen())
      {
        return static_cast<int>(GetFileMaxFilenameLen() + GetFileMaxLinknameLen() + 43);
      }
      return static_cast<int>(GetFileMaxFilenameLen() + 39);

    case ViewMode::MODE_3:
      return static_cast<int>(GetFileMaxFilenameLen() + 3);

    case ViewMode::MODE_4:
      if (GetFileMaxLinknameLen())
      {
        return static_cast<int>(GetFileMaxFilenameLen() + GetFileMaxLinknameLen() + 44);
      }
      return static_cast<int>(GetFileMaxFilenameLen() + 40);

    case ViewMode::MODE_5:
      return static_cast<int>(GetFileMaxUserviewLen() + 1);
  }

  return 1;
}

std::vector<TableColumn> FileTableColumnsForMode(ViewMode mode)
{
  std::vector<TableColumn> cols;
  cols.push_back({"mark", "", false, 0.0f});
  cols.push_back({"name", "Name", true, 0.0f});

  switch (mode)
  {
    case ViewMode::MODE_1:
      cols.push_back({"mode", "Mode", false, 0.0f});
      cols.push_back({"links", "Links", false, 1.0f});
      cols.push_back({"size", "Size", false, 1.0f});
      cols.push_back({"modified", "Modified", false, 0.0f});
      if (ShowLinkColumn())
      {
        cols.push_back({"link", "Link", true, 0.0f});
      }
      break;

    case ViewMode::MODE_2:
      cols.push_back({"inode", "Inode", false, 1.0f});
      cols.push_back({"owner", "Owner", false, 0.0f});
      cols.push_back({"group", "Group", false, 0.0f});
      if (ShowLinkColumn())
      {
        cols.push_back({"link", "Link", true, 0.0f});
      }
      break;

    case ViewMode::MODE_3:
      break;

    case ViewMode::MODE_4:
      cols.push_back({"changed", "Changed", false, 0.0f});
      cols.push_back({"accessed", "Accessed", false, 0.0f});
      if (ShowLinkColumn())
      {
        cols.push_back({"link", "Link", true, 0.0f});
      }
      break;

    case ViewMode::MODE_5:
      cols.push_back({"details", "Details", true, 0.0f});
      break;
  }

  return cols;
}

TableRow FormatFileTableRow(
  const std::shared_ptr<FileEntry>& fe_ptr,
  ViewMode mode
)
{
  TableRow row;
  row.cells.push_back(MarkCell(fe_ptr));
  row.cells.push_back(NameCell(fe_ptr));

  switch (mode)
  {
    case ViewMode::MODE_1:
      if (fe_ptr)
      {
        row.cells.push_back(GetAttributes(fe_ptr->meta.Mode()));
        row.cells.push_back(FormatMetaNlink(fe_ptr->meta));
        row.cells.push_back(std::format("{}", static_cast<std::int64_t>(fe_ptr->meta.size)));
        row.cells.push_back(FormatMetaTime(fe_ptr->meta.mtime));
      }
      else
      {
        row.cells.insert(row.cells.end(), 4, {});
      }
      if (ShowLinkColumn())
      {
        row.cells.push_back(LinkCell(fe_ptr));
      }
      break;

    case ViewMode::MODE_2:
      if (fe_ptr)
      {
        row.cells.push_back(FormatMetaInode(fe_ptr->meta));
        row.cells.push_back(FormatMetaOwner(fe_ptr->meta));
        row.cells.push_back(FormatMetaGroup(fe_ptr->meta));
      }
      else
      {
        row.cells.insert(row.cells.end(), 3, {});
      }
      if (ShowLinkColumn())
      {
        row.cells.push_back(LinkCell(fe_ptr));
      }
      break;

    case ViewMode::MODE_3:
      break;

    case ViewMode::MODE_4:
      if (fe_ptr)
      {
        row.cells.push_back(FormatMetaTime(fe_ptr->meta.ctime));
        row.cells.push_back(FormatMetaTime(fe_ptr->meta.atime));
      }
      else
      {
        row.cells.insert(row.cells.end(), 2, {});
      }
      if (ShowLinkColumn())
      {
        row.cells.push_back(LinkCell(fe_ptr));
      }
      break;

    case ViewMode::MODE_5:
      if (fe_ptr)
      {
        row.cells.push_back(BuildUserFileEntry(
          fe_ptr,
          GetFileMaxFilenameLen(),
          GetFileMaxLinknameLen(),
          GetProfileValueOrEmpty("USERVIEW"),
          200
        ));
      }
      else
      {
        row.cells.emplace_back();
      }
      break;
  }

  return row;
}

std::vector<TableColumn> DirTableColumnsForMode(ViewMode mode)
{
  std::vector<TableColumn> cols;
  cols.push_back({"name", "Name", true, 0.0f});

  switch (mode)
  {
    case ViewMode::MODE_1:
      cols.push_back({"mode", "Mode", false, 0.0f});
      cols.push_back({"links", "Links", false, 1.0f});
      cols.push_back({"size", "Size", false, 1.0f});
      cols.push_back({"modified", "Modified", false, 0.0f});
      break;

    case ViewMode::MODE_2:
      cols.push_back({"inode", "Inode", false, 1.0f});
      cols.push_back({"owner", "Owner", false, 0.0f});
      cols.push_back({"group", "Group", false, 0.0f});
      break;

    case ViewMode::MODE_3:
    case ViewMode::MODE_5:
      break;

    case ViewMode::MODE_4:
      cols.push_back({"changed", "Changed", false, 0.0f});
      cols.push_back({"accessed", "Accessed", false, 0.0f});
      break;
  }

  return cols;
}

TableRow FormatDirTableRow(const DirListRow& row, ViewMode mode)
{
  TableRow out;
  const DirEntry* de = row.dir_entry.get();
  std::string name = (de && !de->name.empty()) ? de->name : ".";

  if (de && de->not_scanned)
  {
    name += "/";
  }
  out.cells.push_back(std::string(row.level * 2, ' ') + name);

  if (!de)
  {
    const auto cols = DirTableColumnsForMode(mode);

    while (out.cells.size() < cols.size())
    {
      out.cells.emplace_back();
    }

    return out;
  }

  switch (mode)
  {
    case ViewMode::MODE_1:
      out.cells.push_back(GetAttributes(de->meta.Mode()));
      out.cells.push_back(FormatMetaNlink(de->meta));
      out.cells.push_back(std::format("{}", static_cast<std::int64_t>(de->meta.size)));
      out.cells.push_back(FormatMetaTime(de->meta.mtime));
      break;

    case ViewMode::MODE_2:
      out.cells.push_back(FormatMetaInode(de->meta));
      out.cells.push_back(FormatMetaOwner(de->meta));
      out.cells.push_back(FormatMetaGroup(de->meta));
      break;

    case ViewMode::MODE_3:
    case ViewMode::MODE_5:
      break;

    case ViewMode::MODE_4:
      out.cells.push_back(FormatMetaTime(de->meta.ctime));
      out.cells.push_back(FormatMetaTime(de->meta.atime));
      break;
  }

  return out;
}
