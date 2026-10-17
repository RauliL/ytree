#include "./ytree.hpp"

static int GetStatFromTAR(
  std::string_view tar_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
);

/**
 * Dateibaum aus TAR-Listing lesen.
 */
bool ReadTreeFromTAR(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingWithDirs(dir_entry, f, GetStatFromTAR, "tarinfo");
}

static int GetStatFromTAR(
  std::string_view tar_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
)
{
  int id;
  std::tm tm_struct{};

  InitArchiveStat(stat);
  symlink_target.clear();

  StringTokenizer tok(tar_line);

  auto t = tok.Next(" \t");
  if (!t) return -1;

  /* Attribute */
  /*-----------*/

  if (t->size() != 10) return -1;
  stat->st_mode = GetModus(std::string(*t));

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

  if (const auto month = ArchiveMonthIndex(*t))
  {
    tm_struct.tm_mon = *month;

    t = tok.Next(" \t");
    if (!t) return -1;
    tm_struct.tm_mday = SvAtoi(*t);

    t = tok.Next(" \t:");
    if (!t) return -1;
    tm_struct.tm_hour = SvAtoi(*t);

    t = tok.Next(" \t");
    if (!t) return -1;
    tm_struct.tm_min = SvAtoi(*t);

    t = tok.Next(" \t");
    if (!t) return -1;
    tm_struct.tm_year = SvAtoi(*t) - 1900;

    t = tok.Next(" \t");
    if (!t) return -1;
  }
  else
  {
    /* YYYY-MM-DD form */
    if (t->size() < 10) return -1;

    tm_struct.tm_year = SvAtoi(t->substr(0, 4)) - 1900;
    tm_struct.tm_mon = SvAtoi(t->substr(5, 2)) - 1;
    tm_struct.tm_mday = SvAtoi(t->substr(8, 2));

    t = tok.Next(" \t:");
    if (!t) return -1;
    tm_struct.tm_hour = SvAtoi(*t);

    t = tok.Next(" \t");
    if (!t) return -1;
    tm_struct.tm_min = SvAtoi(*t);

    t = tok.Next(" \t");
    if (!t) return -1;
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;
  stat->st_mtime = Mktime(&tm_struct);

  /* Dateiname */
  /*-----------*/

  name = *t;

  if (S_ISLNK(stat->st_mode))
  {
    /* Symbolischer Link */
    /*-------------------*/

    t = tok.Next(" \t");
    if (!t) return -1;  /* "->" */
    t = tok.Next(" \t");
    if (!t) return -1;
    symlink_target = *t;
  }

  return 0;
}
