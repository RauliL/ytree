/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/lha.c,v 1.12 2000/05/20 20:41:11 werner Exp $
 *
 * Funktionen zum Lesen des Dateibaumes aus LHA-Dateien
 *
 ***************************************************************************/


#include "ytree.h"



static int GetStatFromLHA(std::string_view lha_line, std::string& name, struct stat *stat);




/* Dateibaum aus LHA-Listing lesen */
/*---------------------------------*/

int ReadTreeFromLHA(const std::shared_ptr<DirEntry>& dir_entry, FILE *f)
{
  char lha_line[LHA_LINE_LENGTH + 1];
  struct stat st;
  bool dir_flag = false;

  dir_entry->name.clear();

  while (std::fgets(lha_line, LHA_LINE_LENGTH, f) != nullptr)
  {
    /* \n loeschen */
    /*-------------*/

    std::string line = lha_line;
    if (!line.empty() && line.back() == '\n')
    {
      line.pop_back();
    }

    if (((line.size() > 55 && line[55] == ':') ||
         (line.size() > 61 && line[61] == ':')) &&
        line[34] != '*' && !std::string_view(line).substr(1).starts_with("Total"))
    {
      /* gueltiger Eintrag */
      /*-------------------*/

      std::string path_name;

      if (GetStatFromLHA(line, path_name, &st))
      {
        FormatMessage("unknown lhainfo*{}", line);
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




static int GetStatFromLHA(std::string_view lha_line, std::string& name, struct stat *stat)
{
  bool dos_mode = false;
  int i, id;
  std::tm tm_struct{};
  static constexpr std::string_view month[] = {
    "Jan", "Feb", "Mar", "Apr", "Mai", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  std::memset(stat, 0, sizeof(struct stat));
  stat->st_nlink = 1;

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

  for (i = 0; i < 12; i++)
  {
    if (*t == month[i]) break;
  }
  if (i >= 12) i = 0;

  tm_struct.tm_mon = i;

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
