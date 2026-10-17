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

std::optional<archive_meta::NamedEntry> archive_meta::ParseLhaListing(
  std::string_view lha_line
)
{
  bool dos_mode = false;
  std::tm tm_struct{};
  NamedEntry entry;
  entry.meta = RegularFile();

  StringTokenizer tok(lha_line);

  auto t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Attributes */
  /*------------*/

  if (t->size() == 9 && (*t)[0] != '[')
  {
    entry.meta.SetFromMode(
      static_cast<std::uint32_t>(GetModus(std::string("-") + std::string(*t)))
    );
  }
  else if ((*t)[0] == '[')
  {
    entry.meta.SetFromMode(
      static_cast<std::uint32_t>(GetModus("-rw-r--r--"))
    );
    dos_mode = true;
  }
  else
  {
    return std::nullopt;
  }

  t = tok.Next(" \t/");
  if (!t) return std::nullopt;

  if (!dos_mode)
  {
    /* Owner */
    /*-------*/

    entry.meta.uid = static_cast<std::uint32_t>(SvAtoi(*t));

    t = tok.Next(" \t");
    if (!t) return std::nullopt;

    /* Group */
    /*-------*/

    entry.meta.gid = static_cast<std::uint32_t>(SvAtoi(*t));

    t = tok.Next(" \t");
    if (!t) return std::nullopt;
  }
  /* DOS listings have no owner — leave uid/gid unset. */

  /* Packed-Size */
  /*-------------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0])))
  {
    return std::nullopt;
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Dateilaenge */
  /*-------------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0])))
  {
    return std::nullopt;
  }
  entry.meta.size = AtoLL(*t);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Ratio */
  /*-------*/

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* CRC */
  /*-----*/

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  if (lha_line.size() > 61 && lha_line[61] == ':')
  {
    /* ??? */
    /*-----*/

    t = tok.Next(" \t");
    if (!t) return std::nullopt;
  }

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mon = ArchiveMonthIndex(*t).value_or(0);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  tm_struct.tm_mday = SvAtoi(*t);

  t = tok.Next(" \t:");
  if (!t) return std::nullopt;

  tm_struct.tm_hour = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  tm_struct.tm_min = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  if (lha_line.size() > 41 && lha_line[41] == '-')
  {
    /* ??? */
    tm_struct.tm_year = 70; /* Start UNIX-Time */
  }
  else
  {
    tm_struct.tm_year = SvAtoi(*t) - 1900;

    t = tok.Next(" \t");
    if (!t) return std::nullopt;
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  /* Dateiname */
  /*-----------*/

  entry.name = *t;

  return entry;
}

static int GetStatFromLHA(std::string_view lha_line, std::string& name, struct stat *stat)
{
  const auto parsed = archive_meta::ParseLhaListing(lha_line);
  if (!parsed)
  {
    return -1;
  }

  name = parsed->name;
  *stat = parsed->meta.ToStat();

  /* Legacy: DOS LHA listings previously faked process owner. */
  if (!parsed->meta.uid)
  {
    stat->st_uid = static_cast<uid_t>(getuid());
    stat->st_gid = static_cast<gid_t>(getgid());
  }

  return 0;
}
