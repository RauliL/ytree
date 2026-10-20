#include "archive/macros.hpp"
#include "core/api.hpp"
#include "core/archive.hpp"
#include "util/string-tokenizer.hpp"

static inline bool IsLHALine(std::string_view line)
{
  return (
    (line.size() > 55 && line[55] == ':') ||
    (line.size() > 61 && line[61] == ':')
  ) &&
    line[34] != '*' &&
    !line.substr(1).starts_with("Total");
}

static std::optional<archive_meta::NamedEntry> ParseLHAListing(
  std::string_view lha_line
)
{
  bool dos_mode = false;
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  StringTokenizer tok(lha_line);
  std::optional<std::string_view> t;

  entry.meta = archive_meta::RegularFile();

  REQUIRE_TOKEN(tok, t, " \t");

  // Attributes

  if (t->size() == 9 && t->front() != '[')
  {
    entry.meta.SetFromMode("-" + std::string(*t));
  }
  else if (t->front() == '[')
  {
    entry.meta.SetFromMode("-rw-r--r--");
    dos_mode = true;
  } else {
    return std::nullopt;
  }

  REQUIRE_TOKEN(tok, t, " \t/");

  if (!dos_mode)
  {
    // Owner

    entry.meta.uid = static_cast<std::uint32_t>(SvAtoi(*t));

    REQUIRE_TOKEN(tok, t, " \t");

    // Group

    entry.meta.gid = static_cast<std::uint32_t>(SvAtoi(*t));

    REQUIRE_TOKEN(tok, t, " \t");
  }
  // DOS listings have no owner — leave uid/gid unset.

  // Packed-Size

  REQUIRE_DIGIT(t);
  REQUIRE_TOKEN(tok, t, " \t");

  // Dateilaenge

  REQUIRE_DIGIT(t);
  entry.meta.size = AtoLL(*t);
  REQUIRE_TOKEN(tok, t, " \t");

  // Ratio

  REQUIRE_TOKEN(tok, t, " \t");

  // CRC

  REQUIRE_TOKEN(tok, t, " \t");

  if (lha_line.size() > 61 && lha_line[61] == ':')
  {
    // ???
    REQUIRE_TOKEN(tok, t, " \t");
  }

  // M-Datum

  tm_struct.tm_mon = ArchiveMonthIndex(*t).value_or(0);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_mday = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t:");

  tm_struct.tm_hour = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_min = SvAtoi(*t);

  REQUIRE_TOKEN(tok, t, " \t");

  if (lha_line.size() > 41 && lha_line[41] == '-')
  {
    // ???
    tm_struct.tm_year = 70; // Start UNIX-Time
  } else {
    tm_struct.tm_year = SvAtoi(*t) - 1900;
    REQUIRE_TOKEN(tok, t, " \t");
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  // Dateiname

  entry.name = *t;

  return entry;
}

/**
 * Dateibaum aus LHA-Listing lesen.
 */
bool ReadTreeFromLHA(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    IsLHALine,
    ParseLHAListing,
    "lhainfo"
  );
}
