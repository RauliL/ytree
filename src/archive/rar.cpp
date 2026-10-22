#include "archive/macros.hpp"
#include "core/api.hpp"
#include "util/string.hpp"
#include "util/string-tokenizer.hpp"

static inline bool LooksLikeOldRARListing(std::string_view line)
{
  return line.size() > 48 && line[48] == ':';
}

static inline bool LooksLikeNewRARListing(std::string_view line)
{
  return line.size() > 36;
}

static inline bool IsRARLine(std::string_view line)
{
  return LooksLikeOldRARListing(line) || LooksLikeNewRARListing(line);
}

static std::optional<archive_meta::NamedEntry> ParseOldRARListing(
  std::string_view rar_line
)
{
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  StringTokenizer tok(rar_line);
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

  // Packed

  REQUIRE_TOKEN(tok, t, " \t");

  // Ratio

  REQUIRE_TOKEN(tok, t, " \t");

  // M-Datum

  if (t->size() == 8)
  {
    tm_struct.tm_mday = ToInt(t->substr(0, 2));
    tm_struct.tm_mon = ToInt(t->substr(3, 2));
    tm_struct.tm_year = ToInt(t->substr(6, 2));

    if (tm_struct.tm_year < 70)
    {
      tm_struct.tm_year += 100;
    }
  }

  REQUIRE_TOKEN(tok, t, " \t");

  // M-time

  if (t->size() == 5)
  {
    tm_struct.tm_hour = ToInt(t->substr(0, 2));
    tm_struct.tm_min = ToInt(t->substr(3, 2));
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = TimePointFromLocalTm(tm_struct);

  return entry;
}

static std::optional<archive_meta::NamedEntry> ParseNewRARListing(
  std::string_view rar_line
)
{
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  StringTokenizer tok(rar_line);
  std::optional<std::string_view> t;

  entry.meta = archive_meta::RegularFile();

  REQUIRE_TOKEN(tok, t, " \t");

  // Optional encrypted/solid marker from modern unrar.
  if (*t == "*")
  {
    REQUIRE_TOKEN(tok, t, " \t");
  }

  // Attributes

  if (t->size() == 7 && (*t)[2] != 'A')
  {
    // Skip non-file DOS/Windows attribute entries.
    entry.name.clear();

    return entry;
  }

  if (t->size() == 10)
  {
    entry.meta.SetFromMode(*t);
  }
  else if (t->size() == 7)
  {
    entry.meta.SetFromMode("-rw-rw-rw-");
  } else {
    entry.name.clear();

    return entry;
  }

  // Directory rows are represented via paths of file members.
  if (!entry.meta.IsRegular())
  {
    entry.name.clear();

    return entry;
  }

  // Size

  REQUIRE_TOKEN(tok, t, " \t");

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0])))
  {
    entry.name.clear();

    return entry;
  }
  entry.meta.size = AtoLL(*t);

  // Date YYYY-MM-DD (may be ????-??-?? for encrypted headers)

  REQUIRE_TOKEN(tok, t, " \t");

  bool have_date = false;
  if (t->size() == 10 && (*t)[0] != '?')
  {
    tm_struct.tm_year = ToInt(t->substr(0, 4)) - 1900;
    tm_struct.tm_mon = ToInt(t->substr(5, 2)) - 1;
    tm_struct.tm_mday = ToInt(t->substr(8, 2));
    have_date = true;
  }

  // Time HH:MM (may be ??:??)

  REQUIRE_TOKEN(tok, t, " \t");

  if (t->size() == 5 && (*t)[2] == ':' && (*t)[0] != '?')
  {
    tm_struct.tm_hour = ToInt(t->substr(0, 2));
    tm_struct.tm_min = ToInt(t->substr(3, 2));
    have_date = true;
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  if (have_date)
  {
    entry.meta.mtime = TimePointFromLocalTm(tm_struct);
  }

  // Filename (may contain spaces; take rest of line)

  if (!(t = tok.Next("")))
  {
    entry.name.clear();

    return entry;
  }

  std::string_view name = *t;
  while (
    !name.empty() &&
    std::isspace(static_cast<unsigned char>(name.front()))
  )
  {
    name.remove_prefix(1);
  }

  if (name.empty())
  {
    entry.name.clear();

    return entry;
  }

  entry.name = name;

  return entry;
}

static std::optional<archive_meta::NamedEntry> ParseRARListing(
  std::string_view rar_line
)
{
  if (LooksLikeOldRARListing(rar_line))
  {
    return ParseOldRARListing(rar_line);
  }
  else if (LooksLikeNewRARListing(rar_line))
  {
    return ParseNewRARListing(rar_line);
  }

  return std::nullopt;
}

/**
 * Dateibaum aus RAR-Listing lesen.
 *
 * Supports classic `unrar l` rows (name first, colon near column 48) and the
 * modern attrs/size/date/time/name layout used by current unrar/rar.
 */
bool ReadTreeFromRAR(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    IsRARLine,
    ParseRARListing,
    "rarinfo"
  );
}
