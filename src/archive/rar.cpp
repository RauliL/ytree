#include "ytree.hpp"

#include <cctype>

static inline bool LooksLikeOldRarListing(std::string_view line)
{
  return line.size() > 48 && line[48] == ':';
}

static bool LooksLikeNewRarListing(std::string_view line)
{
  return line.size() > 36;
}

static std::optional<archive_meta::NamedEntry> ParseOldRarListing(
  std::string_view rar_line
)
{
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  entry.meta = archive_meta::RegularFile();

  StringTokenizer tok(rar_line);

  auto t = tok.Next(" \t");
  if (!t) return std::nullopt;

  // Dateiname

  entry.name = *t;

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  // Dateilaenge

  if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0])))
  {
    return std::nullopt;
  }
  entry.meta.size = AtoLL(*t);

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  // Packed

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  // Ratio

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  // M-Datum

  if (t->size() == 8)
  {
    tm_struct.tm_mday = SvAtoi(t->substr(0, 2));
    tm_struct.tm_mon = SvAtoi(t->substr(3, 2));
    tm_struct.tm_year = SvAtoi(t->substr(6, 2));

    if (tm_struct.tm_year < 70)
    {
      tm_struct.tm_year += 100;
    }
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  // M-time

  if (t->size() == 5)
  {
    tm_struct.tm_hour = SvAtoi(t->substr(0, 2));
    tm_struct.tm_min = SvAtoi(t->substr(3, 2));
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  return entry;
}

static std::optional<archive_meta::NamedEntry> ParseNewRarListing(
  std::string_view rar_line
)
{
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  entry.meta = archive_meta::RegularFile();

  StringTokenizer tok(rar_line);

  auto t = tok.Next(" \t");
  if (!t) return std::nullopt;

  // Optional encrypted/solid marker from modern unrar.
  if (*t == "*")
  {
    t = tok.Next(" \t");
    if (!t) return std::nullopt;
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
    entry.meta.SetFromMode(static_cast<std::uint32_t>(GetModus(std::string(*t))));
  }
  else if (t->size() == 7)
  {
    entry.meta.SetFromMode(
      static_cast<std::uint32_t>(GetModus("-rw-rw-rw-"))
    );
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

  t = tok.Next(" \t");
  if (!t)
  {
    return std::nullopt;
  }
  else if (t->empty() || !std::isdigit(static_cast<unsigned char>((*t)[0])))
  {
    entry.name.clear();

    return entry;
  }
  entry.meta.size = AtoLL(*t);

  // Date YYYY-MM-DD (may be ????-??-?? for encrypted headers)

  t = tok.Next(" \t");
  if (!t)
  {
    return std::nullopt;
  }

  bool have_date = false;
  if (t->size() == 10 && (*t)[0] != '?')
  {
    tm_struct.tm_year = SvAtoi(t->substr(0, 4)) - 1900;
    tm_struct.tm_mon = SvAtoi(t->substr(5, 2)) - 1;
    tm_struct.tm_mday = SvAtoi(t->substr(8, 2));
    have_date = true;
  }

  // Time HH:MM (may be ??:??)

  t = tok.Next(" \t");
  if (!t)
  {
    return std::nullopt;
  }

  if (t->size() == 5 && (*t)[2] == ':' && (*t)[0] != '?')
  {
    tm_struct.tm_hour = SvAtoi(t->substr(0, 2));
    tm_struct.tm_min = SvAtoi(t->substr(3, 2));
    have_date = true;
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  if (have_date)
  {
    entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));
  }

  // Filename (may contain spaces; take rest of line)

  t = tok.Next("");
  if (!t)
  {
    entry.name.clear();

    return entry;
  }

  std::string_view name = *t;
  while (!name.empty() &&
         std::isspace(static_cast<unsigned char>(name.front())))
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

std::optional<archive_meta::NamedEntry> archive_meta::ParseRarListing(
  std::string_view rar_line
)
{
  if (LooksLikeOldRarListing(rar_line))
  {
    return ParseOldRarListing(rar_line);
  }
  else if (LooksLikeNewRarListing(rar_line))
  {
    return ParseNewRarListing(rar_line);
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
    [](std::string_view line)
    {
      return LooksLikeOldRarListing(line) || LooksLikeNewRarListing(line);
    },
    archive_meta::ParseRarListing,
    "rarinfo"
  );
}
