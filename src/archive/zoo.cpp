#include "archive/macros.hpp"
#include "core/api.hpp"
#include "core/archive.hpp"
#include "util/string-tokenizer.hpp"

static inline bool IsZOOLine(std::string_view line)
{
  return line.size() > 50;
}

static std::optional<archive_meta::NamedEntry> ParseZOOListing(
  std::string_view zoo_line
)
{
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  StringTokenizer tok(zoo_line);
  std::optional<std::string_view> t;

  entry.meta = archive_meta::RegularFile();

  REQUIRE_TOKEN(tok, t, " \t");

  // Dateilaenge

  REQUIRE_DIGIT(t);
  entry.meta.size = AtoLL(*t);
  REQUIRE_TOKEN(tok, t, " \t");

  // CF

  REQUIRE_DIGIT(t);
  REQUIRE_TOKEN(tok, t, " \t");

  // Size Now

  REQUIRE_DIGIT(t);
  REQUIRE_TOKEN(tok, t, " \t");

  // M-Datum

  tm_struct.tm_mday = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t:");

  tm_struct.tm_mon = ArchiveMonthIndex(*t).value_or(0);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_year = SvAtoi(*t);
  if (tm_struct.tm_year < 70)
  {
    tm_struct.tm_year += 100;
  }

  REQUIRE_TOKEN(tok, t, " \t:");

  tm_struct.tm_hour = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t:");

  tm_struct.tm_min = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_sec = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  // Attributes

  {
    unsigned int mode = 0;

    std::sscanf(std::string(*t).c_str(), "%o", &mode);
    entry.meta.SetFromMode(mode | S_IFREG);
  }

  REQUIRE_TOKEN(tok, t, " \t");

  // Dateiname

  entry.name = *t;

  return entry;
}

/**
 * Dateibaum aus ZOO-Listing lesen.
 */
bool ReadTreeFromZOO(const std::shared_ptr<DirEntry>& dir_entry, FILE *f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    IsZOOLine,
    ParseZOOListing,
    "zooinfo"
  );
}
