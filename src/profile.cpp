#include "ytree.h"

#include <unordered_map>
#include <vector>

enum class Section
{
  GLOBAL = 1,
  VIEWER = 2,
  MENU = 3,
  FILEMAP = 4,
  FILECMD = 5,
  DIRMAP = 6,
  DIRCMD = 7,
};

struct Profile
{
  const char* def;
  const char* envvar;
  char* value;
};

struct UserAction
{
  int chkey;
  int chremap;
  std::optional<std::string> cmd;
};

static std::vector<UserAction> dirmenu;
static std::vector<UserAction> filemenu;
static std::optional<std::string> custom_profile_path;
static std::unordered_map<std::string, std::string> viewer_mapping;

static std::unordered_map<std::string, Profile> profile =
{
  { { "ARCEXPAND" }, { DEFAULT_ARCEXPAND,     NULL,     NULL } },
  { { "ARCLIST" },    { DEFAULT_ARCLIST,       NULL,     NULL } },
  { { "BUNZIP" },     { DEFAULT_BUNZIP,        NULL,     NULL } },
  { { "CAT" },        { DEFAULT_CAT,           NULL,     NULL } },
  { { "DIR1" },       { DEFAULT_DIR1,          NULL,     NULL } },
  { { "DIR2" },       { DEFAULT_DIR2,          NULL,     NULL } },
  { { "EDITOR" },     { DEFAULT_EDITOR,        "EDITOR", NULL } },
  { { "FILE1" },      { DEFAULT_FILE1,         NULL,     NULL } },
  { { "FILE2" },      { DEFAULT_FILE2,         NULL,     NULL } },
  { { "FILEMODE" },   { DEFAULT_FILEMODE,      NULL,     NULL } },
  { { "GNUUNZIP" },   { DEFAULT_GNUUNZIP,      NULL,     NULL } },
  { { "HEXDUMP" },    { DEFAULT_HEXDUMP,       NULL,     NULL } },
  { { "HEXEDITOFFSET" }, { DEFAULT_HEXEDITOFFSET, NULL,     NULL } },
  { { "INITIALDIR" },    { DEFAULT_INITIALDIR,    NULL,     NULL } },
  { { "LHAEXPAND" },     { DEFAULT_LHAEXPAND,     NULL,     NULL } },
  { { "LHALIST" },       { DEFAULT_LHALIST,       NULL,     NULL } },
  { { "LISTJUMPSEARCH" }, { DEFAULT_LISTJUMPSEARCH,NULL,     NULL } },
  { { "MANROFF" },       { DEFAULT_MANROFF,       NULL,     NULL } },
  { { "MELT" },          { DEFAULT_MELT,          NULL,     NULL } },
  { { "NOSMALLWINDOW" }, { DEFAULT_NOSMALLWINDOW, NULL,     NULL } },
  { { "NUMBERSEP" },     { DEFAULT_NUMBERSEP,     NULL,     NULL } },
  { { "PAGER" },         { DEFAULT_PAGER,        "PAGER",  NULL } },
  { { "RAREXPAND" },     { DEFAULT_RAREXPAND,     NULL,     NULL } },
  { { "RARLIST" },       { DEFAULT_RARLIST,       NULL,     NULL } },
  { { "RPMEXPAND" },     { DEFAULT_RPMEXPAND,     NULL,     NULL } },
  { { "RPMLIST" },       { DEFAULT_RPMLIST,       NULL,     NULL } },
  { { "SEARCHCOMMAND" }, { DEFAULT_SEARCHCOMMAND, NULL,     NULL } },
  { { "TAPEDEV" },       { DEFAULT_TAPEDEV,       "TAPE",   NULL } },
  { { "TAREXPAND" },     { DEFAULT_TAREXPAND,     NULL,     NULL } },
  { { "TARLIST" },       { DEFAULT_TARLIST,       NULL,     NULL } },
  { { "TREEDEPTH" },     { DEFAULT_TREEDEPTH,     NULL,     NULL } },
  { { "UNCOMPRESS" },    { DEFAULT_UNCOMPRESS,    NULL,     NULL } },
  { { "USERVIEW" },      {	"",                    NULL,     NULL } },
  { { "ZIPEXPAND" },     { DEFAULT_ZIPEXPAND,     NULL,     NULL } },
  { { "ZIPLIST" },       { DEFAULT_ZIPLIST,       NULL,     NULL } },
  { { "ZOOEXPAND" },     { DEFAULT_ZOOEXPAND,     NULL,     NULL } },
  { { "ZOOLIST" },       { DEFAULT_ZOOLIST,       NULL,     NULL } },
};

static inline int ChCode(const char*);

static void ParseUserActionMapEntry(
  std::vector<UserAction>& container,
  char* name,
  char* buffer
)
{
  auto value = std::strchr(buffer, '=');
  char* n;
  char* old = nullptr;

  if (*name && value)
  {
    *value++ = 0;
    // Trim whitespace.
    while (*value && std::isspace(*value))
    {
      ++value;
    }
    n = Strtok_r(name, ",", &old);
    // Maybe comma-separated list, e.g. "k,K=x"
    while (n)
    {
      bool found = false;

      // Check for existing entry from FILECMD_SECTION.
      for (auto& entry : filemenu)
      {
        if (entry.chkey == ChCode(n))
        {
          entry.chremap = ChCode(value);
          if (entry.chremap == 0)
          {
            // Don't beep if user cmd defined.
            entry.chremap = -1;
          }
          found = true;
          break;
        }
      }
      if (!found)
      {
        filemenu.push_back({
          ChCode(n),
          ChCode(value),
          std::nullopt
        });
      }
      n = Strtok_r(nullptr, ",",  &old);
    }
  }
}

static void ParseUserActionCmdEntry(
  std::vector<UserAction>& container,
  const char* name,
  char* buffer
)
{
  auto value = std::strchr(buffer, '=');

  if (*name && value)
  {
    bool found = false;

    *value++ = 0;
    // Trim whitespace.
    while (*value && std::isspace(*value))
    {
      ++value;
    }
    // May not be comma-separated list.
    // Check for existing entry from FILEMAP_SECTION.
    for (auto& entry : filemenu)
    {
      if (entry.chkey == ChCode(name))
      {
        entry.cmd = value;
        if (entry.chremap == 0)
        {
          // Don't beep if user cmd defined.
          entry.chremap = -1;
        }
        found = true;
        break;
      }
    }
    if (!found)
    {
      filemenu.push_back({
        ChCode(name),
        ChCode(name),
        value,
      });
    }
  }
}

static std::optional<Section> ParseSectionName(const char* name)
{
  if (!std::strcmp(name, "[GLOBAL]"))
  {
    return Section::GLOBAL;
  }
  else if (!std::strcmp(name, "[VIEWER]"))
  {
    return Section::VIEWER;
  }
  else if (!std::strcmp(name, "[MENU]"))
  {
    return Section::MENU;
  }
  else if (!std::strcmp(name, "[FILEMAP]"))
  {
    return Section::FILEMAP;
  }
  else if (!std::strcmp(name, "[FILECMD]"))
  {
    return Section::FILECMD;
  }
  else if (!std::strcmp(name, "[DIRMAP]"))
  {
    return Section::DIRMAP;
  }
  else if (!std::strcmp(name, "[DIRCMD]"))
  {
    return Section::DIRCMD;
  }

  return std::nullopt;
}

int ReadProfile(const std::optional<std::string>& custom_path)
{
  std::string filename;
  char buffer[BUFSIZ];
  std::optional<Section> section;
  FILE* f;
  int result = -1;

  if (custom_path)
  {
    custom_profile_path = *custom_path;
    filename = *custom_path;
  }
  else if (const auto config_path = GetXdgConfigPath())
  {
    filename = PathJoin({ *config_path, "config.ini" });
  } else {
    goto FNC_XIT;
  }

  if (!(f = std::fopen(filename.c_str(), "r")))
  {
    goto FNC_XIT;
  }

  while (std::fgets(buffer, BUFSIZ, f))
  {
    std::size_t l;

    if (*buffer == '#')
    {
      continue;
    }
    l = std::strlen(buffer);
    if (l > 2)
    {
      char* name;
      char* cptr;

      buffer[l - 1] = 0;
      // Trim whitespace.
      for (name = buffer; std::isspace(*name); ++name);
      for (cptr = name; !std::isspace(*cptr) && *cptr != '='; ++cptr);
      if (*cptr != '=')
      {
        *cptr = 0;
      }
      // Section
      if (*name == '[')
      {
        section = ParseSectionName(name);
        continue;
      }
      if (!section)
      {
        continue;
      }
      else if (*section == Section::GLOBAL)
      {
        auto value = std::strchr(buffer, '=');

        if (*name && value)
        {
          auto entry = profile.find(reinterpret_cast<char*>(name));

          *value++ = 0;
          if (entry != std::end(profile))
          {
            entry->second.value = Strdup(value);
          }
        }
      }
      else if (*section == Section::MENU)
      {
        auto value = std::strchr(buffer, '=');

        if (*name && value)
        {
          *value++ = 0;
          if (
            !std::strcmp(name, "DIR1") ||
            !std::strcmp(name, "DIR2") ||
            !std::strcmp(name, "FILE1") ||
            !std::strcmp(name, "FILE2")
          )
          {
            const auto entry = profile.find(name);

            if (entry != std::end(profile))
            {
              // Space pad menu strings to length COLS, ignoring '(' and ')'
              // characters.
              std::size_t l = 0;

              for (auto cptr = value; *cptr; ++cptr)
              {
                if (*cptr != '(' && *cptr != ')')
                {
                  ++l;
                }
              }
              while (l++ < static_cast<std::size_t>(COLS - 1))
              {
                *cptr++ = ' ';
              }
              *cptr = 0;
              entry->second.value = Strdup(value);
            }
          }
        }
      }
      else if (*section == Section::FILEMAP)
      {
        ParseUserActionMapEntry(filemenu, name, buffer);
      }
      else if (*section == Section::FILECMD)
      {
        ParseUserActionCmdEntry(filemenu, name, buffer);
      }
      else if (*section == Section::DIRMAP)
      {
        ParseUserActionMapEntry(dirmenu, name, buffer);
      }
      else if (*section == Section::DIRCMD)
      {
        ParseUserActionCmdEntry(dirmenu, name, buffer);
      }
      else if (*section == Section::VIEWER)
      {
        auto value = std::strchr(buffer, '=');

        if (*name && value)
        {
          char* n;
          char* old = nullptr;

          *value++ = 0;
          n = Strtok_r(name, ",", &old);
          while (n)
          {
            viewer_mapping[n] = value;
            n = Strtok_r(nullptr, ",", &old);
          }
        }
      }
    }
  }
  result = 0;

FNC_XIT:
  if (f)
  {
    std::fclose(f);
  }

  return result;
}

const char* GetProfileValue(const char* name)
{
  const auto entry = profile.find(name);

  if (entry != std::end(profile))
  {
    if (entry->second.value)
    {
      return entry->second.value;
    }
    else if (entry->second.envvar)
    {
      if (const auto value = std::getenv(entry->second.envvar))
      {
        return value;
      }
    }

    return entry->second.def;
  }

  return "";
}

static inline int ChCode(const char* s)
{
  return *s == '^' && *(s + 1) != '^'
    ? static_cast<int>((*(s + 1)) & 0x1f)
    : static_cast<int>(*s);
}

std::optional<std::string> GetUserAction(
  const std::vector<UserAction>& container,
  int chkey,
  int* pchremap
)
{
  for (const auto& entry : container)
  {
    if (chkey == entry.chkey)
    {
      if (pchremap)
      {
        *pchremap = entry.chremap;
      }

      return entry.cmd;
    }
  }
  if (pchremap)
  {
    *pchremap = chkey;
  }

  return std::nullopt;

}

std::optional<std::string> GetUserFileAction(int chkey, int* pchremap)
{
  return GetUserAction(filemenu, chkey, pchremap);
}

std::optional<std::string> GetUserDirAction(int chkey, int* pchremap)
{
  return GetUserAction(dirmenu, chkey, pchremap);
}

bool IsUserActionDefined()
{
  return !dirmenu.empty() || !dirmenu.empty();
}

std::optional<std::string> GetExtViewer(const std::string& filename)
{
  if (const auto extension = GetExtension(filename))
  {
    const auto entry = viewer_mapping.find('.' + *extension);

    if (entry != std::end(viewer_mapping))
    {
      return entry->second;
    }
  }

  return std::nullopt;
}
