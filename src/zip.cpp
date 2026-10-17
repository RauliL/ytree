#include "./ytree.hpp"

static int GetStatFromZIP(std::string_view zip_line, std::string& name, struct stat *stat);

/**
 * Dateibaum aus ZIP-Listing lesen.
 */
bool ReadTreeFromZIP(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    [](std::string_view line) {
      return line.size() > 58 &&
             (line[56] == ':' || (line[57] != 'd' && line[58] == ':'));
    },
    GetStatFromZIP,
    "zipinfo"
  );
}

static int GetStatFromZIP(std::string_view zip_line, std::string& name, struct stat *stat)
{
  int id;
  std::tm tm_struct{};

  InitArchiveStat(stat);

  StringTokenizer tok(zip_line);

  auto t = tok.Next(" \t");
  if (!t) return -1;

  /* Attributes */
  /*------------*/

  if (t->size() == 10)
  {
    stat->st_mode = GetModus(std::string(*t));
  }
  else
  {
    /* DOS-Zip-File ? */
    /*----------------*/

    stat->st_mode = GetModus("-rw-rw-rw-");
  }

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Version */
  /*---------*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* BS */
  /*----*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Dateilaenge */
  /*-------------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0]))) return -1;
  stat->st_size = AtoLL(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  /* ?? */
  /*----*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Compressed-Laenge */
  /*-------------------*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Methode */
  /*---------*/

  t = tok.Next(" \t-");
  if (!t) return -1;

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mday = SvAtoi(*t);

  t = tok.Next(" \t-");
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

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;
  stat->st_mtime = Mktime(&tm_struct);

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
