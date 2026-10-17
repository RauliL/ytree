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

std::optional<archive_meta::NamedEntry> archive_meta::ParseZooListing(
  std::string_view zoo_line
)
{
  std::tm tm_struct{};
  NamedEntry entry;
  entry.meta = RegularFile();

  StringTokenizer tok(zoo_line);

  auto t = tok.Next(" \t");
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

  /* CF */
  /*----*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0])))
  {
    return std::nullopt;
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Size Now */
  /*----------*/

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0])))
  {
    return std::nullopt;
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mday = SvAtoi(*t);

  t = tok.Next(" \t:");
  if (!t) return std::nullopt;

  tm_struct.tm_mon = ArchiveMonthIndex(*t).value_or(0);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  tm_struct.tm_year = SvAtoi(*t);
  if (tm_struct.tm_year < 70)
    tm_struct.tm_year += 100;

  t = tok.Next(" \t:");
  if (!t) return std::nullopt;

  tm_struct.tm_hour = SvAtoi(*t);

  t = tok.Next(" \t:");
  if (!t) return std::nullopt;

  tm_struct.tm_min = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  tm_struct.tm_sec = SvAtoi(*t);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  /* Attributes */
  /*------------*/

  {
    unsigned int mode = 0;
    std::sscanf(std::string(*t).c_str(), "%o", &mode);
    entry.meta.SetFromMode(mode | S_IFREG);
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Owner/group are not in the listing — leave unset. */

  /* Dateiname */
  /*-----------*/

  entry.name = *t;

  return entry;
}

static int GetStatFromZOO(std::string_view zoo_line, std::string& name, struct stat *stat)
{
  const auto parsed = archive_meta::ParseZooListing(zoo_line);
  if (!parsed)
  {
    return -1;
  }

  name = parsed->name;
  *stat = parsed->meta.ToStat();
  stat->st_uid = static_cast<uid_t>(getuid());
  stat->st_gid = static_cast<gid_t>(getgid());

  return 0;
}
