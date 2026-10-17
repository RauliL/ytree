#include "./ytree.hpp"

static int GetStatFromZOO(std::string_view zoo_line, std::string& name, struct stat *stat);

/**
 * Dateibaum aus ZOO-Listing lesen.
 */
bool ReadTreeFromZOO(const std::shared_ptr<DirEntry>& dir_entry, FILE *f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    [](std::string_view line) {
      return line.size() > 50;
    },
    GetStatFromZOO,
    "zooinfo"
  );
}

static int GetStatFromZOO(std::string_view zoo_line, std::string& name, struct stat *stat)
{
  int id;
  std::tm tm_struct{};

  InitArchiveStat(stat);

  StringTokenizer tok(zoo_line);

  auto t = tok.Next(" \t");
  if (!t) return -1;

  /* Dateilaenge */
  /*-------------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0]))) return -1;
  stat->st_size = AtoLL(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  /* CF */
  /*----*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0]))) return -1;

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

  t = tok.Next(" \t:");
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

  t = tok.Next(" \t:");
  if (!t) return -1;

  tm_struct.tm_min = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  tm_struct.tm_sec = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;
  stat->st_mtime = Mktime(&tm_struct);

  /* Attributes */
  /*------------*/

  {
    unsigned int mode = 0;
    std::sscanf(std::string(*t).c_str(), "%o", &mode);
    stat->st_mode = static_cast<mode_t>(mode) | S_IFREG;
  }

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Owner */
  /*-------*/

  id = static_cast<int>(getuid());
  if (id == -1) id = SvAtoi(*t);
  stat->st_uid = static_cast<uid_t>(id);

  /* Group */
  /*-------*/

  id = static_cast<int>(getgid());
  stat->st_gid = static_cast<gid_t>(id);

  /* Dateiname */
  /*-----------*/

  name = *t;

  return 0;
}
