#include "./ytree.hpp"

static int GetStatFromARC(std::string_view arc_line, std::string& name, struct stat *stat);

/**
 * Dateibaum aus ARC-Listing lesen.
 */
bool ReadTreeFromARC(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    [](std::string_view line) {
      return line.size() > 63 && line[63] == ':';
    },
    GetStatFromARC,
    "arcinfo"
  );
}

static int GetStatFromARC(std::string_view arc_line, std::string& name, struct stat *stat)
{
  int id;
  std::tm tm_struct{};

  InitArchiveStat(stat);

  StringTokenizer tok(arc_line);

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

  /* Stowage */
  /*---------*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* SF */
  /*----*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Size Now */
  /*----------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0]))) return -1;

  t = tok.Next(" \t");
  if (!t) return -1;

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mday = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  tm_struct.tm_mon = ArchiveMonthIndex(*t).value_or(0);

  t = tok.Next(" \t");
  if (!t) return -1;

  tm_struct.tm_year = SvAtoi(*t);
  if (tm_struct.tm_year < 70)
    tm_struct.tm_year += 100;

  t = tok.Next(" \t:");
  if (!t) return -1;

  tm_struct.tm_hour = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  if (!t->empty() && t->back() == 'p')
  {
    tm_struct.tm_hour += 12;
  }

  const auto min_token = t->empty() ? *t : t->substr(0, t->size() - 1);
  tm_struct.tm_min = SvAtoi(min_token);

  t = tok.Next(" \t");
  if (!t) return -1;

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;
  stat->st_mtime = Mktime(&tm_struct);

  /* Attributes */
  /*------------*/

  stat->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  /* Owner */
  /*-------*/

  id = static_cast<int>(getuid());
  if (id == -1) id = SvAtoi(*t);
  stat->st_uid = static_cast<uid_t>(id);

  /* Group */
  /*-------*/

  id = static_cast<int>(getgid());
  stat->st_gid = static_cast<gid_t>(id);

  return 0;
}
