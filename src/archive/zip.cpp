#include "archive/macros.hpp"
#include "core/archive.hpp"
#include "core/api.hpp"
#include "util/string.hpp"
#include "util/string-tokenizer.hpp"

static bool IsZIPLine(std::string_view line)
{
  char lead;

  // zipinfo -l entries have a time colon near column 56/58. Summary lines
  // like "2 files, ... compressed:  56.4%" can match that too, so also
  // require a mode/attribute character at the start. */
  if (line.size() <= 58)
  {
    return false;
  }
  if (!(line[56] == ':' || (line[57] != 'd' && line[58] == ':')))
  {
    return false;
  }

  lead = line.front();

  return (
    lead == '-' ||
    lead == 'd' ||
    lead == 'l' ||
    lead == 'b' ||
    lead == 'c' ||
    lead == 'p' ||
    lead == 's'
  );
}

static std::optional<archive_meta::NamedEntry> ParseZIPListing(
  std::string_view zip_line
)
{
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  StringTokenizer tok(zip_line);
  std::optional<std::string_view> t;

  entry.meta = archive_meta::RegularFile();

  REQUIRE_TOKEN(tok, t, " \t");

  // Attributes

  if (t->size() == 10)
  {
    entry.meta.SetFromMode(*t);
  } else {
    // DOS-Zip-File?
    if (t->size() == 7 && t->front() == 'd')
    {
      entry.meta.SetFromMode(
        static_cast<std::uint32_t>(GetModus("drw-rw-rw-"))
      );
    } else {
      entry.meta.SetFromMode(
        static_cast<std::uint32_t>(GetModus("-rw-rw-rw-"))
      );
    }
  }

  // Skip directory-only entries; paths are created from file members.
  if (!entry.meta.IsRegular())
  {
    entry.name.clear();

    return entry;
  }

  REQUIRE_TOKEN(tok, t, " \t");

  // Version (or summary "files," / "file," if the filter missed it)

  if (*t == "file," || *t == "files,")
  {
    entry.name.clear();

    return entry;
  }

  REQUIRE_TOKEN(tok, t, " \t");

  // BS

  REQUIRE_TOKEN(tok, t, " \t");

  // Dateilaenge

  REQUIRE_DIGIT(t);
  entry.meta.size = AtoLL(*t);
  REQUIRE_TOKEN(tok, t, " \t");

  // ??

  REQUIRE_TOKEN(tok, t, " \t");

  // Compressed-Laenge

  REQUIRE_TOKEN(tok, t, " \t");

  // Methode

  REQUIRE_TOKEN(tok, t, " \t-");

  // M-Datum

  tm_struct.tm_mday = ToInt(*t);

  REQUIRE_TOKEN(tok, t, " \t-");

  tm_struct.tm_mon = ArchiveMonthIndex(*t).value_or(0);

  REQUIRE_TOKEN(tok, t, " \t");

  tm_struct.tm_year = ToInt(*t);
  if (tm_struct.tm_year < 70)
  {
    tm_struct.tm_year += 100;
  }

  REQUIRE_TOKEN(tok, t, " \t:");

  tm_struct.tm_hour = ToInt(*t);

  REQUIRE_TOKEN(tok, t, " \t:");

  tm_struct.tm_min = ToInt(*t);
  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = TimePointFromLocalTm(tm_struct);

  // Dateiname (may contain spaces; take rest of line)

  REQUIRE_TOKEN(tok, t, "");

  entry.name = *t;

  return entry;
}

/**
 * Dateibaum aus ZIP-Listing lesen.
 */
bool ReadTreeFromZIP(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    IsZIPLine,
    ParseZIPListing,
    "zipinfo"
  );
}
