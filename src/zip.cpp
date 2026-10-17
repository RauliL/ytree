#include "./ytree.hpp"

#include "./entry_meta.hpp"

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

std::optional<archive_meta::NamedEntry> archive_meta::ParseZipListing(
  std::string_view zip_line
)
{
  std::tm tm_struct{};
  NamedEntry entry;
  entry.meta = RegularFile();

  StringTokenizer tok(zip_line);

  auto t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Attributes */
  /*------------*/

  if (t->size() == 10)
  {
    entry.meta.SetFromMode(static_cast<std::uint32_t>(GetModus(std::string(*t))));
  }
  else
  {
    /* DOS-Zip-File ? */
    /*----------------*/

    entry.meta.SetFromMode(
      static_cast<std::uint32_t>(GetModus("-rw-rw-rw-"))
    );
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Version */
  /*---------*/

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* BS */
  /*----*/

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

  /* ?? */
  /*----*/

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Compressed-Laenge */
  /*-------------------*/

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Methode */
  /*---------*/

  t = tok.Next(" \t-");
  if (!t) return std::nullopt;

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mday = SvAtoi(*t);

  t = tok.Next(" \t-");
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

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  /* Only mtime is present in zipinfo output; leave atime/ctime unset. */
  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  /* Owner/group are not in the listing — leave unset (no getuid() fake). */

  /* Dateiname */
  /*-----------*/

  entry.name = *t;

  return entry;
}

static int GetStatFromZIP(std::string_view zip_line, std::string& name, struct stat *stat)
{
  const auto parsed = archive_meta::ParseZipListing(zip_line);

  if (!parsed)
  {
    return -1;
  }

  name = parsed->name;
  *stat = parsed->meta.ToStat();

  /* Legacy display: zipinfo has no owner; keep previous fake until UI uses EntryMeta. */
  stat->st_uid = static_cast<uid_t>(getuid());
  stat->st_gid = static_cast<gid_t>(getgid());

  return 0;
}
