#include <algorithm>

#include "./ytree.hpp"

static bool GetArchiveDirEntry(
  const std::shared_ptr<DirEntry>& tree,
  const char* path,
  DirEntry** dir_entry
);

static bool InsertArchiveDirEntry(
  const std::shared_ptr<DirEntry>& tree,
  const std::string& path,
  const EntryMeta& meta
)
{
  DirEntry* df_ptr;
  const auto sep = std::filesystem::path::preferred_separator;

#ifdef DEBUG
  std::fprintf(stderr, "Insert Dir \"%s\"\n", path.c_str());
#endif

  // Format: .../dir/

  std::string father_path = path;
  const auto trailing_sep = father_path.rfind(sep);

  if (trailing_sep == std::string::npos)
  {
    FormatError("patch mismatch*missing '{}' in*{}", sep, path);

    return false;
  }
  father_path.resize(trailing_sep);

  const auto parent_sep = father_path.rfind(sep);
  std::string name;

  if (parent_sep == std::string::npos)
  {
    df_ptr = tree.get();
    if (path.size() == 1 && path[0] == sep)
    {
      name = path;
    } else {
      name = father_path;
    }
  } else {
    name = father_path.substr(parent_sep + 1);
    father_path.resize(parent_sep + 1);  /* keep trailing separator */

    if (!GetArchiveDirEntry(tree, father_path.c_str(), &df_ptr))
    {
      FormatError("can't find subdir*{}", father_path);

      return false;
    }
  }

  auto de_ptr = std::make_shared<DirEntry>();
  de_ptr->name = std::move(name);
  de_ptr->meta = meta;

#ifdef DEBUG
  std::fprintf(stderr, "new dir: \"%s\"\n", de_ptr->name.c_str());
#endif

  // Directory einklinken

  /* Entweder direkt in tree (= df_ptr) oder in dessen Unterverzeichnis
   * (= df_ptr); die Unterverzeichnisse bleiben dabei nach Namen sortiert.
   */
  de_ptr->parent = WeakDirEntry(df_ptr);

  df_ptr->children.insert(
    std::find_if(
      df_ptr->children.begin(),
      df_ptr->children.end(),
      [&de_ptr](const std::shared_ptr<DirEntry>& ds_ptr) {
        return ds_ptr->name > de_ptr->name;
      }
    ),
    de_ptr
  );

  statistic.disk_total_directories++;

  return true;
}

int InsertArchiveFileEntry(
  const std::shared_ptr<DirEntry>& tree,
  const std::string& path,
  const EntryMeta& meta,
  std::string symlink_target
)
{
  DirEntry *de_ptr;

  if (KeyPressed())
  {
    Quit();  /* Abfrage, ob ytree verlassen werden soll */
  }

  const auto split = Fnsplit(path);

  if (!GetArchiveDirEntry(tree, split.dir.c_str(), &de_ptr))
  {
#ifdef DEBUG
    std::fprintf(stderr, "can't get directory for file*%s*trying recover",
                 path.c_str());
#endif

    if (!TryInsertArchiveDirEntry(tree, split.dir.c_str(), archive_meta::Directory()))
    {
      Error("Inserting directory failed");
      return -1;
    }
    if (!GetArchiveDirEntry(tree, split.dir.c_str(), &de_ptr))
    {
      FormatError("again: can't get directory for file*{}*giving up", path);
      return -1;
    }
  }

  auto fe_ptr = std::make_shared<FileEntry>();
  fe_ptr->meta = meta;
  fe_ptr->name = split.name;

  if (meta.IsLink())
  {
    fe_ptr->symlink_target = std::move(symlink_target);
  }

  fe_ptr->dir_entry = WeakDirEntry(de_ptr);
  de_ptr->total_files++;
  de_ptr->total_bytes += meta.size;
  statistic.disk_total_files++;
  statistic.disk_total_bytes += meta.size;

  // Einklinken

  de_ptr->files.push_back(fe_ptr);

  return 0;
}

static bool GetArchiveDirEntry(
  const std::shared_ptr<DirEntry>& tree,
  const char* path,
  DirEntry** dir_entry
)
{
  bool is_root = false;

#ifdef DEBUG
  std::fprintf(
    stderr,
    "GetArchiveDirEntry: tree=%s, path=%s\n",
    tree ? tree->name : "NULL",
    path
  );
#endif

  if (std::strchr(path, std::filesystem::path::preferred_separator))
  {
    for (const auto& de_ptr : tree->children)
    {
      const int n = de_ptr->name.size();

      if (
        de_ptr->name.size() == 1 &&
        de_ptr->name[0] == std::filesystem::path::preferred_separator
      )
      {
        is_root = true;
      }

      if (
        n &&
        !de_ptr->name.compare(0, n, path, n) &&
        (
          is_root ||
          path[n] == '\0' ||
          path[n] == std::filesystem::path::preferred_separator
        )
      )
      {
        if (
          (is_root && path[n] == '\0') ||
          (
            path[n] == std::filesystem::path::preferred_separator &&
            path[n+1] == '\0'
          )
        )
        {
          // Pfad abgearbeitet; ==> fertig
          *dir_entry = de_ptr.get();

          return true;
        } else {
          return GetArchiveDirEntry(
            de_ptr,
            is_root ? &path[n] : &path[n + 1],
            dir_entry
          );
        }
      }
    }
  }
  if (*path == '\0')
  {
    *dir_entry = tree.get();

    return true;
  }

  return false;
}

bool TryInsertArchiveDirEntry(
  const std::shared_ptr<DirEntry>& tree,
  const char* dir,
  const EntryMeta& meta
)
{
  DirEntry *de_ptr;
  char dir_path[PATH_LENGTH + 1];
  const char *s;
  char *t;

  std::memset(dir_path, 0, sizeof( dir_path ));

#ifdef DEBUG
  std::fprintf(stderr, "Try install start \n");
#endif

  for (s = dir, t = dir_path; *s; s++, t++)
  {
    if ((*t = *s) == std::filesystem::path::preferred_separator)
    {
      if (!GetArchiveDirEntry(tree, dir_path, &de_ptr))
      {
        // Evtl. fehlender teil; ==> einfuegen
        if (!InsertArchiveDirEntry(tree, dir_path, meta))
        {
          return false;
        }
      }
    }
  }

#ifdef DEBUG
  std::fprintf(stderr, "Try install end\n");
#endif

  return true;
}

void FinalizeArchiveTree(const std::shared_ptr<DirEntry>& dir_entry)
{
  statistic.disk_total_directories++;
  dir_entry->meta = archive_meta::Directory();
  MinimizeArchiveTree(dir_entry);
}

bool IsArchiveDirectoryEntry(const std::string& path, const EntryMeta& meta)
{
  return meta.IsDir() ||
         path == "." ||
         (!path.empty() &&
          path.back() == std::filesystem::path::preferred_separator);
}

bool ReadArchiveListingFilesOnly(
  const std::shared_ptr<DirEntry>& dir_entry,
  FILE* f,
  ArchiveLineFilter is_entry,
  ArchiveMetaParser parse,
  std::string_view unknown_tag
)
{
  dir_entry->name.clear();

  while (auto line = ReadLine(f))
  {
    if (!is_entry(*line))
    {
      continue;
    }

    const auto parsed = parse(*line);

    if (!parsed)
    {
      FormatMessage("unknown {}*{}", unknown_tag, *line);
    }
    else if (!parsed->name.empty())
    {
#ifdef DEBUG
      std::fprintf(stderr, "FILE: \"%s\"\n", parsed->name.c_str());
#endif
      InsertArchiveFileEntry(
        dir_entry,
        parsed->name,
        parsed->meta,
        parsed->symlink_target
      );
    }
  }

  FinalizeArchiveTree(dir_entry);

  return true;
}

bool ReadArchiveListingWithDirs(
  const std::shared_ptr<DirEntry>& dir_entry,
  FILE* f,
  ArchiveMetaParser parse,
  std::string_view unknown_tag
)
{
  dir_entry->name.clear();

  while (auto line = ReadLine(f))
  {
    auto parsed = parse(*line);

    if (!parsed)
    {
      FormatMessage("unknown {}*{}", unknown_tag, *line);
    }
    else if (IsArchiveDirectoryEntry(parsed->name, parsed->meta))
    {
#ifdef DEBUG
      std::fprintf(stderr, "DIR: %s\n", parsed->name.c_str());
#endif
      if (parsed->name != "./")
      {
        TryInsertArchiveDirEntry(dir_entry, parsed->name.c_str(), parsed->meta);
        DisplayDiskStatistic();
        doupdate();
      }
    } else {
#ifdef DEBUG
      std::fprintf(stderr, "FILE: \"%s\"\n", parsed->name.c_str());
#endif
      InsertArchiveFileEntry(
        dir_entry,
        parsed->name,
        parsed->meta,
        std::move(parsed->symlink_target)
      );
    }
  }

  FinalizeArchiveTree(dir_entry);

  return true;
}

void MinimizeArchiveTree(const std::shared_ptr<DirEntry>& tree)
{
  /* tree ist ein namenloser Platzhalter, dessen Unterverzeichnisse die
   * obersten Verzeichnisse des Archivs sind.
   *
   * Falls tree genau ein Unterverzeichnis hat und tree selbst keine Dateien
   * enthaelt, wird tree durch dieses Unterverzeichnis ersetzt. Bei mehreren
   * obersten Verzeichnissen (oder Dateien in tree) bleibt der Platzhalter
   * als Wurzel bestehen.
   */
  if (tree->files.empty() && tree->children.size() == 1)
  {
    /* Keep the child alive while it is copied over its former owner. */
    const std::shared_ptr<DirEntry> child = tree->children.front();

    *tree = *child;
    tree->parent.reset();
    statistic.disk_total_directories--;
    for (const auto& fe_ptr : tree->files)
    {
      fe_ptr->dir_entry = tree;
    }
    for (const auto& de_ptr : tree->children)
    {
      de_ptr->parent = tree;
    }
  } else {
    return;
  }


  // Test, ob das einzige Unterverzeichnis keine Dateien hat
  while( tree->children.size() == 1 && tree->children.front()->files.empty() )
  {
    // Zusammenfassung moeglich
    const std::shared_ptr<DirEntry> de_ptr = tree->children.front();

    if( !(tree->name.size() == 1 &&
          tree->name[0] == std::filesystem::path::preferred_separator) )
    {
      tree->name += std::filesystem::path::preferred_separator;
    }
    tree->name += de_ptr->name;
    statistic.disk_total_directories--;
    tree->children = std::move(de_ptr->children);
    for (const auto& de1_ptr : tree->children)
    {
      de1_ptr->parent = tree;
    }
#ifdef DEBUG
    std::fprintf(stderr, "new root-dir: \"%s\"\n", tree->name.c_str());
#endif
  }

  /* Letzter Optimierungsschritt:
   * Falls tree keine Dateien, aber genau einen Subtree hat, wird
   * zusammengefasst
   */
  if (tree->files.empty() && tree->children.size() == 1)
  {
    const std::shared_ptr<DirEntry> de_ptr = tree->children.front();

    tree->name += std::filesystem::path::preferred_separator;
    tree->name += de_ptr->name;
    tree->files = std::move(de_ptr->files);
    for (const auto& fe_ptr : tree->files)
    {
      fe_ptr->dir_entry = tree;
    }
    tree->meta = de_ptr->meta;
    statistic.disk_total_directories--;
    tree->children = std::move(de_ptr->children);
    for (const auto& de1_ptr : tree->children)
    {
      de1_ptr->parent = tree;
    }
  }
}

std::string MakeExtractCommandLine(
  const std::string& path,
  const std::string& file,
  const std::string& cmd
)
{
  const auto l = path.length();

  if (const auto compress_method = GetFileMethod(path))
  {
    if (*compress_method == CompressMethod::ZOO_COMPRESS)
    {
      /* zoo xp FILE ?? */
      /*----------------*/
      return std::format(
        "{} '{}' '{}' {}",
        GetProfileValueOrEmpty("ZOOEXPAND"),
        path,
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::LHA_COMPRESS)
    {
      /* xlharc p FILE ?? */
      /*------------------*/
      return std::format(
        "{} '{}' '{}' {}",
        GetProfileValueOrEmpty("LHAEXPAND"),
        path,
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::ZIP_COMPRESS)
    {
      /* unzip -c FILE ?? */
      /*------------------*/
      return std::format(
        "{} '{}' '{}' {}",
        GetProfileValueOrEmpty("ZIPEXPAND"),
        path,
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::ARC_COMPRESS)
    {
      /* arc p FILE ?? */
      /*---------------*/
      return std::format(
        "{} '{}' '{}' {}",
        GetProfileValueOrEmpty("ARCEXPAND"),
        path,
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::RPM_COMPRESS)
    {
      const auto expand = GetProfileValueOrEmpty("RPMEXPAND");

      /* TF=/tmp/ytree.$$; mkdir $TF; cd $TF; rpm2cpio RPM_FILE | cpio -id FILE;
       * cat $TF/$2; cd /tmp; rm -rf $TF; exit 0
       */
      if (expand == "builtin")
      {
        return std::format(
          "(TF=/tmp/ytree.$$; mkdir $TF; rpm2cpio '{}' | (cd $TF; cpio --no-absolute-filenames -i -d '{}'); cat \"$TF/{}\"; cd /tmp; rm -rf $TF; exit 0) {}",
          path,
          !file.empty() && file[0] == std::filesystem::path::preferred_separator ? file.substr(1) : file,
          file,
          cmd
        );
      } else {
        return std::format(
          "{} '{}' '{}' {}",
          expand,
          path,
          file,
          cmd
        );
      }
    }
    else if (*compress_method == CompressMethod::RAR_COMPRESS)
    {
      /* rar p FILE ?? */
      /*---------------*/
      return std::format(
        "{} '{}' '{}' {}",
        GetProfileValueOrEmpty("RAREXPAND"),
        path,
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::SEVENZIP_COMPRESS)
    {
      /* 7z x -so ARCHIVE FILE ?? */
      /*--------------------------*/
      return std::format(
        "{} '{}' '{}' {}",
        GetProfileValueOrEmpty("SEVENZIPEXPAND"),
        path,
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::FREEZE_COMPRESS)
    {
      /* melt < TAR_FILE | gtar xOf - FILE ?? */
      /*--------------------------------------*/
      return std::format(
        "{} < '{}' | {} '{}' {}",
        GetProfileValueOrEmpty("MELT"),
        path,
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::MULTIPLE_FREEZE_COMPRESS)
    {
      /* CAT TAR_FILEs | melt | gtar xOf - FILE ?? */
      /*-------------------------------------------*/
      const auto cat = path.substr(0, l - 2) + "*";

      return std::format(
        "{} {} | {} | {} '{}' {}",
        GetProfileValueOrEmpty("CAT"),
        cat,
        GetProfileValueOrEmpty("MELT"),
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::COMPRESS_COMPRESS)
    {
      /* uncompress < TAR_FILE | gtar xOf - FILE ?? */
      /*--------------------------------------------*/
      return std::format(
        "{} < {} | {} '{}' {}",
        GetProfileValueOrEmpty("UNCOMPRESS"),
        path,
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::MULTIPLE_COMPRESS_COMPRESS)
    {
      /* CAT TAR_FILEs | uncompress | gtar xOf - FILE ?? */
      /*-------------------------------------------------*/
      const auto cat = path.substr(0, l - 2) + "*";

      return std::format(
        "{} {} | {} | {} '{}' {}",
        GetProfileValueOrEmpty("CAT"),
        cat,
        GetProfileValueOrEmpty("UNCOMPRESS"),
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::GZIP_COMPRESS)
    {
      /* gunzip < TAR_FILE | gtar xOf - FILE ?? */
      /*----------------------------------------*/
      return std::format(
        "{} < '{}' | {} '{}' {}",
        GetProfileValueOrEmpty("GNUUNZIP"),
        path,
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::MULTIPLE_GZIP_COMPRESS)
    {
      /* CAT TAR_FILEs | gunzip | gtar xOf - FILE ?? */
      /*---------------------------------------------*/
      const auto cat = path.substr(0, l - 2) + "*";

      return std::format(
        "{} {} | {} | {} '{}' {}",
        GetProfileValueOrEmpty("CAT"),
        cat,
        GetProfileValueOrEmpty("GNUUNZIP"),
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::BZIP_COMPRESS)
    {
      /* bunzip2 < TAR_FILE | gtar xOf - FILE ?? */
      /*----------------------------------------*/
      return std::format(
        "{} < '{}' | {} '{}' {}",
        GetProfileValueOrEmpty("BUNZIP"),
        path,
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::XZ_COMPRESS)
    {
      /* xz -dc < TAR_FILE | gtar xOf - FILE ?? */
      /*---------------------------------------*/
      return std::format(
        "{} < '{}' | {} '{}' {}",
        GetProfileValueOrEmpty("UNXZ"),
        path,
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
    else if (*compress_method == CompressMethod::ZSTD_COMPRESS)
    {
      /* zstd -dc < TAR_FILE | gtar xOf - FILE ?? */
      /*-----------------------------------------*/
      return std::format(
        "{} < '{}' | {} '{}' {}",
        GetProfileValueOrEmpty("UNZSTD"),
        path,
        GetProfileValueOrEmpty("TAREXPAND"),
        file,
        cmd
      );
    }
  }

  /* gtar xOf - FILE < TAR_FILE ?? */
  /*-------------------------------*/
  return std::format(
    "{} '{}' < '{}' {}",
    GetProfileValueOrEmpty("TAREXPAND"),
    file,
    path,
    cmd
  );
}
