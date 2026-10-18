#include "./ytree.hpp"

/**
 * Dateibaum aus TAR-Listing lesen.
 */
bool ReadTreeFromTAR(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingWithDirs(
    dir_entry,
    f,
    archive_meta::ParseTarListing,
    "tarinfo"
  );
}

std::optional<archive_meta::NamedEntry> archive_meta::ParseTarListing(
  std::string_view tar_line
)
{
  int id;
  std::tm tm_struct{};
  NamedEntry entry;

  StringTokenizer tok(tar_line);

  auto t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Attribute */
  /*-----------*/

  if (t->size() != 10) return std::nullopt;
  entry.meta.SetFromMode(static_cast<std::uint32_t>(GetModus(std::string(*t))));
  entry.meta.nlink = 1;

  t = tok.Next(" \t/");
  if (!t) return std::nullopt;

  /* Owner */
  /*-------*/
  if (const auto owner_id_ptr = GetPasswdUid(std::string(*t)))
  {
    id = *owner_id_ptr;
  }
  else
  {
    id = SvAtoi(*t);
  }
  entry.meta.uid = static_cast<std::uint32_t>(id);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Group */
  /*-------*/
  if (const auto group_id_ptr = GetGroupId(std::string(*t)))
  {
    id = *group_id_ptr;
  }
  else
  {
    id = SvAtoi(*t);
  }
  entry.meta.gid = static_cast<std::uint32_t>(id);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Dateilaenge */
  /*-------------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0])))
  {
    return std::nullopt;
  }
  entry.meta.size = AtoLL(*t);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* M-Datum */
  /*---------*/

  if (const auto month = ArchiveMonthIndex(*t))
  {
    tm_struct.tm_mon = *month;

    t = tok.Next(" \t");
    if (!t) return std::nullopt;
    tm_struct.tm_mday = SvAtoi(*t);

    t = tok.Next(" \t:");
    if (!t) return std::nullopt;
    tm_struct.tm_hour = SvAtoi(*t);

    t = tok.Next(" \t");
    if (!t) return std::nullopt;
    tm_struct.tm_min = SvAtoi(*t);

    t = tok.Next(" \t");
    if (!t) return std::nullopt;
    tm_struct.tm_year = SvAtoi(*t) - 1900;
  }
  else
  {
    /* YYYY-MM-DD form */
    if (t->size() < 10) return std::nullopt;

    tm_struct.tm_year = SvAtoi(t->substr(0, 4)) - 1900;
    tm_struct.tm_mon = SvAtoi(t->substr(5, 2)) - 1;
    tm_struct.tm_mday = SvAtoi(t->substr(8, 2));

    t = tok.Next(" \t:");
    if (!t) return std::nullopt;
    tm_struct.tm_hour = SvAtoi(*t);

    t = tok.Next(" \t");
    if (!t) return std::nullopt;
    tm_struct.tm_min = SvAtoi(*t);
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  /* Dateiname (may contain spaces; take rest of line) */
  /*---------------------------------------------------*/

  t = tok.Next("");
  if (!t) return std::nullopt;

  if (entry.meta.IsLink())
  {
    /* Symbolischer Link: "name -> target" or "name symbolic link to target" */
    /*---------------------------------------------------------------------*/

    const auto full = *t;
    constexpr std::string_view gnu_arrow = " -> ";
    constexpr std::string_view non_gnu_arrow = " symbolic link to ";

    if (const auto pos = full.find(gnu_arrow); pos != std::string_view::npos)
    {
      entry.name = full.substr(0, pos);
      entry.symlink_target = full.substr(pos + gnu_arrow.size());
    }
    else if (const auto pos = full.find(non_gnu_arrow);
             pos != std::string_view::npos)
    {
      entry.name = full.substr(0, pos);
      entry.symlink_target = full.substr(pos + non_gnu_arrow.size());
    }
    else
    {
      entry.name = full;
    }
  }
  else
  {
    entry.name = *t;
  }

  return entry;
}
