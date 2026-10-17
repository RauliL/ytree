#include "./ytree.hpp"

static int GetStatFromRAR(std::string_view rar_line, std::string& name, struct stat *stat);

/**
 * Dateibaum aus RAR-Listing lesen.
 */
bool ReadTreeFromRAR(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    [](std::string_view line) {
      return line.size() > 48 && line[48] == ':';
    },
    GetStatFromRAR,
    "rarinfo"
  );
}

static int GetStatFromRAR(std::string_view rar_line, std::string& name, struct stat *stat)
{
  int id;
  std::tm tm_struct{};

  InitArchiveStat(stat);

  StringTokenizer tok(rar_line);

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

  /* Packed */
  /*--------*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* Ratio */
  /*-------*/

  t = tok.Next(" \t");
  if (!t) return -1;

  /* M-Datum */
  /*---------*/

  if (t->size() == 8)
  {
    tm_struct.tm_mday = SvAtoi(t->substr(0, 2));
    tm_struct.tm_mon = SvAtoi(t->substr(3, 2));
    tm_struct.tm_year = SvAtoi(t->substr(6, 2));

    if (tm_struct.tm_year < 70)
      tm_struct.tm_year += 100;
  }

  t = tok.Next(" \t");
  if (!t) return -1;

  /* M-time */
  /*--------*/

  if (t->size() == 5)
  {
    tm_struct.tm_hour = SvAtoi(t->substr(0, 2));
    tm_struct.tm_min = SvAtoi(t->substr(3, 2));
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;
  stat->st_mtime = Mktime(&tm_struct);

  t = tok.Next(" \t");
  if (!t) return -1;

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
