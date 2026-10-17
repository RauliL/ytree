#include "./ytree.hpp"

/**
 * Dateibaum aus RAR-Listing lesen.
 */
bool ReadTreeFromRAR(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingFilesOnly(
    dir_entry,
    f,
    [](std::string_view line) {
      return line.size() > 48 && line[48] == ':';
    },
    archive_meta::ParseRarListing,
    "rarinfo"
  );
}

std::optional<archive_meta::NamedEntry> archive_meta::ParseRarListing(
  std::string_view rar_line
)
{
  std::tm tm_struct{};
  NamedEntry entry;
  entry.meta = RegularFile();

  StringTokenizer tok(rar_line);

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

  /* Packed */
  /*--------*/

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* Ratio */
  /*-------*/

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* M-Datum */
  /*---------*/

  if (t->size() == 8)
  {
    tm_struct.tm_mday = SvAtoi(t->substr(0, 2));
    tm_struct.tm_mon = SvAtoi(t->substr(3, 2));
    tm_struct.tm_year = SvAtoi(t->substr(6, 2));

    if (tm_struct.tm_year < 70)
      tm_struct.tm_year += 100;
  }

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  /* M-time */
  /*--------*/

  if (t->size() == 5)
  {
    tm_struct.tm_hour = SvAtoi(t->substr(0, 2));
    tm_struct.tm_min = SvAtoi(t->substr(3, 2));
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  t = tok.Next(" \t");
  if (!t) return std::nullopt;

  return entry;
}
