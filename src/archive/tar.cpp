#include "archive/macros.hpp"
#include "core/api.hpp"
#include "core/archive.hpp"
#include "util/string-tokenizer.hpp"

static std::optional<archive_meta::NamedEntry> ParseTARListing(
  std::string_view tar_line
)
{
  int id;
  std::tm tm_struct{};
  archive_meta::NamedEntry entry;
  StringTokenizer tok(tar_line);
  std::optional<std::string_view> t;

  REQUIRE_TOKEN(tok, t, " \t");

  // Attribute

  REQUIRE_SIZE(t, 10);
  entry.meta.SetFromMode(*t);
  entry.meta.nlink = 1;

  REQUIRE_TOKEN(tok, t, " \t/");

  // Owner
  if (const auto owner_id_ptr = GetPasswdUid(std::string(*t)))
  {
    id = *owner_id_ptr;
  } else {
    id = SvAtoi(*t);
  }
  entry.meta.uid = static_cast<std::uint32_t>(id);

  REQUIRE_TOKEN(tok, t, " \t");

  // Group
  if (const auto group_id_ptr = GetGroupId(std::string(*t)))
  {
    id = *group_id_ptr;
  } else {
    id = SvAtoi(*t);
  }
  entry.meta.gid = static_cast<std::uint32_t>(id);

  REQUIRE_TOKEN(tok, t, " \t");

  // Dateilaenge

  REQUIRE_DIGIT(t);
  entry.meta.size = AtoLL(*t);

  REQUIRE_TOKEN(tok, t, " \t");

  // M-Datum

  if (const auto month = ArchiveMonthIndex(*t))
  {
    tm_struct.tm_mon = *month;

    REQUIRE_TOKEN(tok, t, " \t");
    tm_struct.tm_mday = SvAtoi(*t);

    REQUIRE_TOKEN(tok, t, " \t:");
    tm_struct.tm_hour = SvAtoi(*t);

    REQUIRE_TOKEN(tok, t, " \t");
    tm_struct.tm_min = SvAtoi(*t);

    REQUIRE_TOKEN(tok, t, " \t");
    tm_struct.tm_year = SvAtoi(*t) - 1900;
  } else {
    // YYYY-MM-DD form
    if (t->size() < 10)
    {
      return std::nullopt;
    }

    tm_struct.tm_year = SvAtoi(t->substr(0, 4)) - 1900;
    tm_struct.tm_mon = SvAtoi(t->substr(5, 2)) - 1;
    tm_struct.tm_mday = SvAtoi(t->substr(8, 2));

    REQUIRE_TOKEN(tok, t, " \t:");
    tm_struct.tm_hour = SvAtoi(*t);

    REQUIRE_TOKEN(tok, t, " \t");
    tm_struct.tm_min = SvAtoi(*t);
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  entry.meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));

  // Dateiname (may contain spaces; take rest of line)

  REQUIRE_TOKEN(tok, t, "");

  if (entry.meta.IsLink())
  {
    // Symbolischer Link: "name -> target" or "name symbolic link to target"
    const auto full = *t;
    constexpr std::string_view gnu_arrow = " -> ";
    constexpr std::string_view non_gnu_arrow = " symbolic link to ";

    if (const auto pos = full.find(gnu_arrow); pos != std::string_view::npos)
    {
      entry.name = full.substr(0, pos);
      entry.symlink_target = full.substr(pos + gnu_arrow.size());
    }
    else if (
      const auto pos = full.find(non_gnu_arrow);
      pos != std::string_view::npos
    )
    {
      entry.name = full.substr(0, pos);
      entry.symlink_target = full.substr(pos + non_gnu_arrow.size());
    } else {
      entry.name = full;
    }
  } else {
    entry.name = *t;
  }

  return entry;
}

/**
 * Dateibaum aus TAR-Listing lesen.
 */
bool ReadTreeFromTAR(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingWithDirs(
    dir_entry,
    f,
    ParseTARListing,
    "tarinfo"
  );
}
