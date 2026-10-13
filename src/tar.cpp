#include "./ytree.hpp"

static int GetStatFromTAR(
  std::string_view tar_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
);

/* Dateibaum aus TAR-Listing lesen */
/*---------------------------------*/
int ReadTreeFromTAR(const std::shared_ptr<DirEntry>& dir_entry, FILE *f)
{
  struct stat st;
  bool dir_flag = false;

  dir_entry->name.clear();

  while (auto line = ReadLine(f))
  {
    std::string path_name;
    std::string symlink_target;

    if (GetStatFromTAR(*line, path_name, symlink_target, &st))
    {
      FormatMessage("unknown tarinfo*{}", *line);
    }
    else
    {
      if (
        (!path_name.empty() &&
         path_name.back() == std::filesystem::path::preferred_separator) ||
        path_name == "." ||
        line->starts_with('d')
      )
      {
        /* Directory */
        /*-----------*/

#ifdef DEBUG
        std::fprintf(stderr, "DIR: %s\n", path_name.c_str());
#endif

        if (path_name != "./")
        {
          /* "./" wird ignoriert */
          /*---------------------*/

          TryInsertArchiveDirEntry(dir_entry, path_name.c_str(), &st);
          DisplayDiskStatistic();
          doupdate();
        }
      }
      else
      {
        /* File */
        /*------*/

#ifdef DEBUG
        std::fprintf(stderr, "FILE: \"%s\"\n", path_name.c_str());
#endif
        InsertArchiveFileEntry(dir_entry, path_name, &st, std::move(symlink_target));
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




static int GetStatFromTAR(
  std::string_view tar_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
)
{
  int i, id;
  std::tm tm_struct{};
  static constexpr std::string_view month[] = {
    "Jan", "Feb", "Mar", "Apr", "Mai", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  std::memset(stat, 0, sizeof(struct stat));
  stat->st_nlink = 1;
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

  for (i = 0; i < 12; i++)
  {
    if (*t == month[i]) break;
  }
  if (i >= 12)
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
    goto XDATE;
  }

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
  tm_struct.tm_year = SvAtoi(*t) - 1900;

  t = tok.Next(" \t");
  if (!t) return -1;

XDATE:

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
