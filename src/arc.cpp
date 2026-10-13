#include "./ytree.hpp"

static int GetStatFromARC(std::string_view arc_line, std::string& name, struct stat *stat);

/* Dateibaum aus ARC-Listing lesen */
/*---------------------------------*/

int ReadTreeFromARC(const std::shared_ptr<DirEntry>& dir_entry, FILE *f)
{
  struct stat st;
  bool dir_flag = false;

  dir_entry->name.clear();

  while (auto line = ReadLine(f))
  {
    if (line->size() > 63 && (*line)[63] == ':')
    {
      /* gueltiger Eintrag */
      /*-------------------*/

      std::string path_name;

      if (GetStatFromARC(*line, path_name, &st))
      {
        FormatMessage("unknown arcinfo*{}", *line);
      }
      else
      {
        /* File */
        /*------*/

#ifdef DEBUG
        std::fprintf(stderr, "FILE: \"%s\"\n", path_name.c_str());
#endif
        InsertArchiveFileEntry(dir_entry, path_name, &st);
      }
    }
  }

  if (dir_flag == false)
  {
    statistic.disk_total_directories++;
    std::memset(&dir_entry->stat_struct, 0, sizeof(struct stat));
    dir_entry->stat_struct.st_mode = S_IFDIR;
  }
  return MinimizeArchiveTree(dir_entry);
}




static int GetStatFromARC(std::string_view arc_line, std::string& name, struct stat *stat)
{
  int i, id;
  std::tm tm_struct{};
  static constexpr std::string_view month[] = {
    "Jan", "Feb", "Mar", "Apr", "Mai", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  std::memset(stat, 0, sizeof(struct stat));
  stat->st_nlink = 1;

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

  for (i = 0; i < 12; i++)
  {
    if (*t == month[i]) break;
  }
  if (i >= 12) i = 0;

  tm_struct.tm_mon = i;

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
