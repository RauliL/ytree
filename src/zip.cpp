#include "./archive.hpp"
#include "./ytree.hpp"

/**
 * Dateibaum aus ZIP-Listing lesen.
 */
bool ReadTreeFromZIP(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    [](std::string_view line) {
      /* zipinfo -l entries have a time colon near column 56/58. Summary lines
       * like "2 files, ... compressed:  56.4%" can match that too, so also
       * require a mode/attribute character at the start. */
      if (line.size() <= 58)
      {
        return false;
      }
      if (!(line[56] == ':' || (line[57] != 'd' && line[58] == ':')))
      {
        return false;
      }
      const char lead = line.front();
      return lead == '-' || lead == 'd' || lead == 'l' || lead == 'b' ||
             lead == 'c' || lead == 'p' || lead == 's';
    },
    archive_meta::ParseZipListing,
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

    if (t->size() == 7 && t->front() == 'd')
    {
      entry.meta.SetFromMode(
        static_cast<std::uint32_t>(GetModus("drw-rw-rw-"))
      );
    }
    else
    {
      entry.meta.SetFromMode(
        static_cast<std::uint32_t>(GetModus("-rw-rw-rw-"))
      );
    }
  }

  /* Skip directory-only entries; paths are created from file members. */
  if (!entry.meta.IsRegular())
  {
    entry.name.clear();
    return entry;
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Version (or summary "files," / "file," if the filter missed it) */
  /*----------------------------------------------------------------*/

  if (*t == "file," || *t == "files,")
  {
    entry.name.clear();
    return entry;
  }

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

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  /* Dateiname (may contain spaces; take rest of line) */
  /*---------------------------------------------------*/

  t = tok.Next("");
  if (!t) return std::nullopt;

  entry.name = *t;

  return entry;
}
