#include "ytree.h"

#include <toml++/toml.hpp>

#include <sstream>
#include <unordered_map>
#include <vector>

struct Profile
{
  const char* def;
  std::optional<std::string> envvar;
  std::optional<std::string> value;
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
  { { "ARCEXPAND" }, { DEFAULT_ARCEXPAND,     std::nullopt,     std::nullopt } },
  { { "ARCLIST" },    { DEFAULT_ARCLIST,       std::nullopt,     std::nullopt } },
  { { "BUNZIP" },     { DEFAULT_BUNZIP,        std::nullopt,     std::nullopt } },
  { { "CAT" },        { DEFAULT_CAT,           std::nullopt,     std::nullopt } },
  { { "DIR1" },       { DEFAULT_DIR1,          std::nullopt,     std::nullopt } },
  { { "DIR2" },       { DEFAULT_DIR2,          std::nullopt,     std::nullopt } },
  { { "EDITOR" },     { DEFAULT_EDITOR,        "EDITOR", std::nullopt } },
  { { "FILE1" },      { DEFAULT_FILE1,         std::nullopt,     std::nullopt } },
  { { "FILE2" },      { DEFAULT_FILE2,         std::nullopt,     std::nullopt } },
  { { "FILEMODE" },   { DEFAULT_FILEMODE,      std::nullopt,     std::nullopt } },
  { { "GNUUNZIP" },   { DEFAULT_GNUUNZIP,      std::nullopt,     std::nullopt } },
  { { "HEXDUMP" },    { DEFAULT_HEXDUMP,       std::nullopt,     std::nullopt } },
  { { "HEXEDITOFFSET" }, { DEFAULT_HEXEDITOFFSET, std::nullopt,     std::nullopt } },
  { { "INITIALDIR" },    { DEFAULT_INITIALDIR,    std::nullopt,     std::nullopt } },
  { { "LHAEXPAND" },     { DEFAULT_LHAEXPAND,     std::nullopt,     std::nullopt } },
  { { "LHALIST" },       { DEFAULT_LHALIST,       std::nullopt,     std::nullopt } },
  { { "LISTJUMPSEARCH" }, { DEFAULT_LISTJUMPSEARCH,std::nullopt,     std::nullopt } },
  { { "MANROFF" },       { DEFAULT_MANROFF,       std::nullopt,     std::nullopt } },
  { { "MELT" },          { DEFAULT_MELT,          std::nullopt,     std::nullopt } },
  { { "NOSMALLWINDOW" }, { DEFAULT_NOSMALLWINDOW, std::nullopt,     std::nullopt } },
  { { "NUMBERSEP" },     { DEFAULT_NUMBERSEP,     std::nullopt,     std::nullopt } },
  { { "PAGER" },         { DEFAULT_PAGER,        "PAGER",  std::nullopt } },
  { { "RAREXPAND" },     { DEFAULT_RAREXPAND,     std::nullopt,     std::nullopt } },
  { { "RARLIST" },       { DEFAULT_RARLIST,       std::nullopt,     std::nullopt } },
  { { "RPMEXPAND" },     { DEFAULT_RPMEXPAND,     std::nullopt,     std::nullopt } },
  { { "RPMLIST" },       { DEFAULT_RPMLIST,       std::nullopt,     std::nullopt } },
  { { "SEARCHCOMMAND" }, { DEFAULT_SEARCHCOMMAND, std::nullopt,     std::nullopt } },
  { { "TAPEDEV" },       { DEFAULT_TAPEDEV,       "TAPE",   std::nullopt } },
  { { "TAREXPAND" },     { DEFAULT_TAREXPAND,     std::nullopt,     std::nullopt } },
  { { "TARLIST" },       { DEFAULT_TARLIST,       std::nullopt,     std::nullopt } },
  { { "TREEDEPTH" },     { DEFAULT_TREEDEPTH,     std::nullopt,     std::nullopt } },
  { { "UNCOMPRESS" },    { DEFAULT_UNCOMPRESS,    std::nullopt,     std::nullopt } },
  { { "USERVIEW" },      {	"",                    std::nullopt,     std::nullopt } },
  { { "ZIPEXPAND" },     { DEFAULT_ZIPEXPAND,     std::nullopt,     std::nullopt } },
  { { "ZIPLIST" },       { DEFAULT_ZIPLIST,       std::nullopt,     std::nullopt } },
  { { "ZOOEXPAND" },     { DEFAULT_ZOOEXPAND,     std::nullopt,     std::nullopt } },
  { { "ZOOLIST" },       { DEFAULT_ZOOLIST,       std::nullopt,     std::nullopt } },
};

static inline int ChCode(const char* s)
{
  return *s == '^' && *(s + 1) != '^'
    ? static_cast<int>((*(s + 1)) & 0x1f)
    : static_cast<int>(*s);
}

static std::string TomlValueToString(const toml::node& node)
{
  if (const auto* value = node.as_string())
  {
    return std::string{**value};
  }
  if (const auto* value = node.as_integer())
  {
    return std::to_string(**value);
  }
  if (const auto* value = node.as_boolean())
  {
    return **value ? "1" : "0";
  }
  if (const auto* value = node.as_floating_point())
  {
    std::ostringstream out;
    out << **value;
    return out.str();
  }

  return {};
}

static inline std::string TomlValueToString(const toml::node_view<const toml::node>& view)
{
  if (!view)
  {
    return {};
  }

  return TomlValueToString(*view.node());
}

static void ApplyKeyMap(
  std::vector<UserAction>& container,
  const std::string& key,
  const std::string& value
)
{
  const auto chkey = ChCode(key.c_str());
  auto chremap = ChCode(value.c_str());

  if (chremap == 0)
  {
    chremap = -1;
  }

  for (auto& entry : container)
  {
    if (entry.chkey == chkey)
    {
      entry.chremap = chremap;
      return;
    }
  }

  container.push_back({ chkey, chremap, std::nullopt });
}

static void ApplyKeyCmd(
  std::vector<UserAction>& container,
  const std::string& key,
  const std::string& value
)
{
  const auto chkey = ChCode(key.c_str());

  for (auto& entry : container)
  {
    if (entry.chkey == chkey)
    {
      entry.cmd = value;
      if (entry.chremap == 0)
      {
        entry.chremap = -1;
      }
      return;
    }
  }

  container.push_back({ chkey, chkey, value });
}

static void ApplyKeyTable(
  std::vector<UserAction>& container,
  const toml::table* table,
  bool is_command
)
{
  if (!table)
  {
    return;
  }

  for (const auto& [key, node] : *table)
  {
    const auto key_str = std::string{key.str()};
    const auto value = TomlValueToString(node);

    if (is_command)
    {
      ApplyKeyCmd(container, key_str, value);
    } else {
      ApplyKeyMap(container, key_str, value);
    }
  }
}

static void ApplyMenuValue(const std::string& name, std::string value)
{
  const auto entry = profile.find(name);

  if (entry == std::end(profile))
  {
    return;
  }

  std::size_t visible = 0;
  for (const auto ch : value)
  {
    if (ch != '(' && ch != ')')
    {
      ++visible;
    }
  }
  if (visible < static_cast<std::size_t>(COLS - 1))
  {
    value.append(static_cast<std::size_t>(COLS - 1) - visible, ' ');
  }
  entry->second.value = std::move(value);
}

static std::string NormalizeExtension(std::string extension)
{
  if (!extension.empty() && extension.front() != '.')
  {
    extension.insert(extension.begin(), '.');
  }
  return extension;
}

int ReadProfile(const std::optional<std::string>& custom_path)
{
  std::filesystem::path filename;

  if (custom_path)
  {
    custom_profile_path = *custom_path;
    filename = *custom_path;
  }
  else if (const auto config_path = GetXdgConfigPath())
  {
    filename = *config_path / "config.toml";
  } else {
    return -1;
  }

  toml::table table;
  try
  {
    table = toml::parse_file(filename.string());
  }
  catch (const toml::parse_error&)
  {
    return -1;
  }

  if (const auto* global = table["global"].as_table())
  {
    for (const auto& [key, node] : *global)
    {
      const auto name = std::string{key.str()};
      auto entry = profile.find(name);

      if (entry != std::end(profile))
      {
        entry->second.value = TomlValueToString(node);
      }
    }
  }

  if (const auto* menu = table["menu"].as_table())
  {
    for (const auto& name : { "DIR1", "DIR2", "FILE1", "FILE2" })
    {
      if ((*menu)[name])
      {
        ApplyMenuValue(name, TomlValueToString((*menu)[name]));
      }
    }
  }

  if (const auto* viewers = table["viewer"].as_array())
  {
    for (const auto& node : *viewers)
    {
      const auto* row = node.as_table();
      if (!row)
      {
        continue;
      }

      const auto command = TomlValueToString((*row)["command"]);
      const auto* extensions = (*row)["extensions"].as_array();
      if (!extensions || command.empty())
      {
        continue;
      }

      for (const auto& extension_node : *extensions)
      {
        if (const auto* extension = extension_node.as_string())
        {
          viewer_mapping[NormalizeExtension(std::string{**extension})] = command;
        }
      }
    }
  }

  ApplyKeyTable(filemenu, table["filemap"].as_table(), false);
  ApplyKeyTable(filemenu, table["filecmd"].as_table(), true);
  ApplyKeyTable(dirmenu, table["dirmap"].as_table(), false);
  ApplyKeyTable(dirmenu, table["dircmd"].as_table(), true);

  return 0;
}

const char* GetProfileValue(const char* name)
{
  const auto entry = profile.find(name);

  if (entry != std::end(profile))
  {
    if (entry->second.value)
    {
      return entry->second.value->c_str();
    }
    else if (entry->second.envvar)
    {
      if (const auto value = std::getenv(entry->second.envvar->c_str()))
      {
        return value;
      }
    }

    return entry->second.def;
  }

  return "";
}

static std::optional<std::string> GetUserAction(
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
  return !dirmenu.empty() || !filemenu.empty();
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
