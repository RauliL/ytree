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

static int GetStatFromRPM(
  std::string_view rpm_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
)
{
  int id;

  InitArchiveStat(stat);
  symlink_target.clear();

  StringTokenizer tok(rpm_line);

  auto t = tok.Next(" \t");
  if (!t) return -1;

  /* Dateiname */
  /*-----------*/

  name = *t;

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Dateilaenge */
  /*-------------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0]))) return -1;
  stat->st_size = AtoLL(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  /* M-Datum */
  /*---------*/

  stat->st_atime = 0;
  stat->st_ctime = 0;
  stat->st_mtime = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  /* M5 */
  if (t->size() > 20)
  {
    t = tok.Next(" \t/");
    if (!t) return -1;
  }

  /* Attribute */
  /*-----------*/

  stat->st_mode = static_cast<mode_t>(
    std::strtoul(std::string(*t).c_str(), nullptr, 8)
  );

  t = tok.Next(" \t/");
  if (!t) return -1;

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
  stat->st_uid = static_cast<uid_t>(id);

  t = tok.Next(" \t");
  if (!t) return -1;

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
  stat->st_gid = static_cast<gid_t>(id);

  if (S_ISLNK(stat->st_mode))
  {
    /* Symbolischer Link */
    /*-------------------*/

    symlink_target = "symlink";
  }

  return 0;
}
