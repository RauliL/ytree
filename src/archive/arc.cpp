#include "archive/macros.hpp"
#include "core/api.hpp"
#include "core/archive.hpp"
#include "util/string-tokenizer.hpp"

static inline bool IsARCEntry(std::string_view line)
{
  return line.size() > 63 && line[63] == ':';
}

static std::optional<archive_meta::NamedEntry> ParseARCListing(
  std::string_view arc_line
)
{
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  StringTokenizer tok(arc_line);
  std::optional<std::string_view> t;

  entry.meta = archive_meta::RegularFile();

  REQUIRE_TOKEN(tok, t, " \t");

  // Dateiname

  entry.name = *t;

  REQUIRE_TOKEN(tok, t, " \t");

  // Dateilaenge

  REQUIRE_DIGIT(t);
  entry.meta.size = AtoLL(*t);

  REQUIRE_TOKEN(tok, t, " \t");

  // Stowage

  REQUIRE_TOKEN(tok, t, " \t");

  // SF

  REQUIRE_TOKEN(tok, t, " \t");

  // Size Now

  REQUIRE_DIGIT(t);

  REQUIRE_TOKEN(tok, t, " \t");

  // M-Datum

  tm_struct.tm_mday = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_mon = ArchiveMonthIndex(*t).value_or(0);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_year = SvAtoi(*t);
  if (tm_struct.tm_year < 70)
  {
    tm_struct.tm_year += 100;
  }

  REQUIRE_TOKEN(tok, t, " \t:");

  tm_struct.tm_hour = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t");

  if (!t->empty() && t->back() == 'p')
  {
    tm_struct.tm_hour += 12;
  }

  const auto min_token = t->empty() ? *t : t->substr(0, t->size() - 1);
  tm_struct.tm_min = SvAtoi(min_token);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = TimePointFromLocalTm(tm_struct);

  return entry;
}

/**
 * Dateibaum aus ARC-Listing lesen.
 */
bool ReadTreeFromARC(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    IsARCEntry,
    ParseARCListing,
    "arcinfo"
  );
}
