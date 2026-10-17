#include "./ytree.hpp"

static int GetStatFromRPM(
  std::string_view rpm_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
);

/**
 * Dateibaum aus RPM-Listing lesen.
 */
bool ReadTreeFromRPM(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingWithDirs(dir_entry, f, GetStatFromRPM, "rpminfo");
}

std::optional<archive_meta::NamedEntry> archive_meta::ParseRpmListing(
  std::string_view rpm_line
)
{
  int id;
  NamedEntry entry;
  entry.meta.nlink = 1;

  StringTokenizer tok(rpm_line);

  auto t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Dateiname */
  /*-----------*/

  entry.name = *t;

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

  {
    const auto mtime = static_cast<std::time_t>(SvAtoi(*t));
    if (mtime != 0)
    {
      entry.meta.mtime = std::chrono::system_clock::from_time_t(mtime);
    }
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* M5 */
  if (t->size() > 20)
  {
    t = tok.Next(" \t/");
    if (!t) return std::nullopt;
  }

  /* Attribute */
  /*-----------*/

  entry.meta.SetFromMode(static_cast<std::uint32_t>(
    std::strtoul(std::string(*t).c_str(), nullptr, 8)
  ));

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

  if (entry.meta.IsLink())
  {
    /* Symbolischer Link */
    /*-------------------*/

    entry.symlink_target = "symlink";
  }

  return entry;
}

static int GetStatFromRPM(
  std::string_view rpm_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
)
{
  const auto parsed = archive_meta::ParseRpmListing(rpm_line);
  if (!parsed)
  {
    return -1;
  }

  name = parsed->name;
  symlink_target = parsed->symlink_target;
  *stat = parsed->meta.ToStat();

  return 0;
}
