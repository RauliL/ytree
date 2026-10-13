#include "./ytree.hpp"

static int GetStatFromRPM(
  std::string_view rpm_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
);

/* Dateibaum aus RPM-Listing lesen */
/*---------------------------------*/
int ReadTreeFromRPM(const std::shared_ptr<DirEntry>& dir_entry, FILE *f)
{
  struct stat st;
  bool dir_flag = false;

  dir_entry->name.clear();

  while (auto line = ReadLine(f))
  {
    std::string path_name;
    std::string symlink_target;

    if (GetStatFromRPM(*line, path_name, symlink_target, &st))
    {
      FormatMessage("unknown rpminfo*{}", *line);
    }
    else
    {
      if (
        (!path_name.empty() &&
         path_name.back() == std::filesystem::path::preferred_separator) ||
        path_name == "." ||
        S_ISDIR(st.st_mode)
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




static int GetStatFromRPM(
  std::string_view rpm_line,
  std::string& name,
  std::string& symlink_target,
  struct stat *stat
)
{
  int id;

  std::memset(stat, 0, sizeof(struct stat));
  stat->st_nlink = 1;
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
