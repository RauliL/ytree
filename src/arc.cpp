#include "./ytree.hpp"

/**
 * Dateibaum aus ARC-Listing lesen.
 */
bool ReadTreeFromARC(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    [](std::string_view line) {
      return line.size() > 63 && line[63] == ':';
    },
    archive_meta::ParseArcListing,
    "arcinfo"
  );
}

std::optional<archive_meta::NamedEntry> archive_meta::ParseArcListing(
  std::string_view arc_line
)
{
  std::tm tm_struct{};
  NamedEntry entry;
  entry.meta = RegularFile();

  StringTokenizer tok(arc_line);

  auto t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Dateiname */
  /*-----------*/

  entry.name = *t;

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

  /* Stowage */
  /*---------*/

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* SF */
  /*----*/

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

  t = tok.Next(" \t");
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

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  if (!t->empty() && t->back() == 'p')
  {
    tm_struct.tm_hour += 12;
  }

  const auto min_token = t->empty() ? *t : t->substr(0, t->size() - 1);
  tm_struct.tm_min = SvAtoi(min_token);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  return entry;
}
