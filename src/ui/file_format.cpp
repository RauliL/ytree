#include "ui/file_format.hpp"

#include "core/api.hpp"
#include "ui/ui.hpp"

#include <format>

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
