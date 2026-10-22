#include "archive/macros.hpp"
#include "core/api.hpp"
#include "util/string.hpp"
#include "util/string-tokenizer.hpp"

static std::optional<archive_meta::NamedEntry> ParseRPMListing(
  std::string_view rpm_line
)
{
  int id;
  archive_meta::NamedEntry entry;
  StringTokenizer tok(rpm_line);
  std::optional<std::string_view> t;

  entry.meta.nlink = 1;

  REQUIRE_TOKEN(tok, t, " \t");

  // Dateiname

  entry.name = *t;

  REQUIRE_TOKEN(tok, t, " \t");

  // Dateilaenge

  REQUIRE_DIGIT(t);
  entry.meta.size = AtoLL(*t);
  REQUIRE_TOKEN(tok, t, " \t");

  // M-Datum

  {
    const auto mtime = ToInt<std::time_t>(*t);

    if (mtime != 0)
    {
      entry.meta.mtime = std::chrono::system_clock::from_time_t(mtime);
    }
  }

  REQUIRE_TOKEN(tok, t, " \t");

  // M5
  if (t->size() > 20)
  {
    REQUIRE_TOKEN(tok, t, " \t/");
  }

  // Attribute

  entry.meta.SetFromMode(static_cast<std::uint32_t>(
    std::strtoul(std::string(*t).c_str(), nullptr, 8)
  ));

  REQUIRE_TOKEN(tok, t, " \t/");

  // Owner
  if (const auto owner_id_ptr = GetPasswdUid(std::string(*t)))
  {
    id = *owner_id_ptr;
  } else {
    id = ToInt(*t);
  }
  entry.meta.uid = static_cast<std::uint32_t>(id);

  REQUIRE_TOKEN(tok, t, " \t");

  // Group
  if (const auto group_id_ptr = GetGroupId(std::string(*t)))
  {
    id = *group_id_ptr;
  } else {
    id = ToInt(*t);
  }
  entry.meta.gid = static_cast<std::uint32_t>(id);

  if (entry.meta.IsLink())
  {
    // Symbolischer Link
    entry.symlink_target = "symlink";
  }

  return entry;
}

/**
 * Dateibaum aus RPM-Listing lesen.
 */
bool ReadTreeFromRPM(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  return ReadArchiveListingWithDirs(
    dir_entry,
    f,
    ParseRPMListing,
    "rpminfo"
  );
}
