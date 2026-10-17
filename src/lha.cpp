#include "./ytree.hpp"

static int GetStatFromLHA(std::string_view lha_line, std::string& name, struct stat *stat);

/**
 * Dateibaum aus LHA-Listing lesen.
 */
bool ReadTreeFromLHA(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    [](std::string_view line) {
      return ((line.size() > 55 && line[55] == ':') ||
              (line.size() > 61 && line[61] == ':')) &&
             line[34] != '*' &&
             !line.substr(1).starts_with("Total");
    },
    GetStatFromLHA,
    "lhainfo"
  );
}

static int GetStatFromLHA(std::string_view lha_line, std::string& name, struct stat *stat)
{
  bool dos_mode = false;
  int id;
  std::tm tm_struct{};

  InitArchiveStat(stat);

  StringTokenizer tok(lha_line);

  auto t = tok.Next(" \t");
  if (!t) return -1;

  /* Attributes */
  /*------------*/

  if (t->size() == 9 && (*t)[0] != '[')
  {
    stat->st_mode = GetModus(std::string("-") + std::string(*t));
  }
  else if ((*t)[0] == '[')
  {
    stat->st_mode = GetModus("-rw-r--r--");
    dos_mode = true;
  }
  else
  {
    return -1;
  }

  t = tok.Next(" \t/");
  if (!t) return -1;

  if (dos_mode)
  {
    stat->st_uid = getuid();
    stat->st_gid = getgid();
  }
  else
  {
    /* Owner */
    /*-------*/

    id = SvAtoi(*t);
    stat->st_uid = static_cast<uid_t>(id);

    t = tok.Next(" \t");
    if (!t) return -1;

    /* Group */
    /*-------*/

    id = SvAtoi(*t);
    stat->st_gid = static_cast<gid_t>(id);

    t = tok.Next(" \t");
    if (!t) return -1;
  }

  /* Packed-Size */
  /*-------------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0]))) return -1;

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Dateilaenge */
  /*-------------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0]))) return -1;
  stat->st_size = AtoLL(*t);

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Ratio */
  /*-------*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* CRC */
  /*-----*/

  t = tok.Next(" \t");
  if (!t) return -1;

  if (lha_line.size() > 61 && lha_line[61] == ':')
  {
    /* ??? */
    /*-----*/

    t = tok.Next(" \t");
    if (!t) return -1;
  }

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mon = ArchiveMonthIndex(*t).value_or(0);

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

  if (lha_line.size() > 41 && lha_line[41] == '-')
  {
    /* ??? */
    tm_struct.tm_year = 70; /* Start UNIX-Time */
  }
  else
  {
    tm_struct.tm_year = SvAtoi(*t) - 1900;

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

  return 0;
}
