#include <cstring>

#include "core/api.hpp"
#include "core/tilde.hpp"

namespace {

const char* default_prefixes[] = { " ~", "\t~", nullptr };
const char* default_suffixes[] = { " ", "\n", nullptr };

const char** tilde_additional_prefixes = default_prefixes;
const char** tilde_additional_suffixes = default_suffixes;

/* Find the start of a tilde expansion in STRING, and return the index of
   the tilde which starts the expansion.  Place the length of the text
   which identified this tilde starter in LEN, excluding the tilde itself. */
int tilde_find_prefix(const char* string, int* len)
{
  const auto string_len = static_cast<int>(std::strlen(string));
  *len = 0;

  if (*string == '\0' || *string == '~')
  {
    return 0;
  }

  if (tilde_additional_prefixes)
  {
    for (int i = 0; i < string_len; ++i)
    {
      for (int j = 0; tilde_additional_prefixes[j]; ++j)
      {
        const auto prefix_len = static_cast<int>(std::strlen(tilde_additional_prefixes[j]));
        if (std::strncmp(string + i, tilde_additional_prefixes[j], prefix_len) == 0)
        {
          *len = prefix_len - 1;
          return i + *len;
        }
      }
    }
  }
  return string_len;
}

/* Find the end of a tilde expansion in STRING, and return the index of
   the character which ends the tilde definition.  */
int tilde_find_suffix(const char* string)
{
  const auto string_len = static_cast<int>(std::strlen(string));

  for (int i = 0; i < string_len; ++i)
  {
    if (string[i] == '/')
    {
      return i;
    }

    for (int j = 0; tilde_additional_suffixes && tilde_additional_suffixes[j]; ++j)
    {
      const auto suffix_len = static_cast<int>(std::strlen(tilde_additional_suffixes[j]));
      if (std::strncmp(string + i, tilde_additional_suffixes[j], suffix_len) == 0)
      {
        return i;
      }
    }
  }
  return string_len;
}

/* Take FNAME and return the tilde prefix we want expanded.  If LENP is
   non-null, the index of the end of the prefix into FNAME is returned in
   the location it points to. */
std::string isolate_tilde_prefix(const char* fname, int* lenp)
{
  int i = 1;
  while (fname[i] && fname[i] != '/')
  {
    ++i;
  }
  if (lenp)
  {
    *lenp = i;
  }
  return { fname + 1, fname + i };
}

std::string glue_prefix_and_suffix(const char* prefix, const char* suffix, int suffind)
{
  std::string result;
  if (prefix && *prefix)
  {
    result = prefix;
  }
  result += suffix + suffind;
  return result;
}

const char* sh_get_home_dir()
{
  const auto pw = getpwuid(getuid());

  if (!pw)
  {
    std::abort();
  }

  return pw->pw_dir;
}

} // namespace

/* Do the work of tilde expansion on FILENAME.  FILENAME starts with a
   tilde.  This always returns a new string. */
std::string tilde_expand_word(const std::string& filename)
{
  if (filename.empty())
  {
    return {};
  }
  if (filename[0] != '~')
  {
    return filename;
  }

  /* A leading `~/' or a bare `~' is *always* translated to the value of
     $HOME or the home directory of the current user. */
  if (filename.size() == 1 || filename[1] == '/')
  {
    const char* expansion = std::getenv("HOME");
    if (!expansion)
    {
      expansion = sh_get_home_dir();
    }
    return glue_prefix_and_suffix(expansion, filename.c_str(), 1);
  }

  int user_len = 0;
  const auto username = isolate_tilde_prefix(filename.c_str(), &user_len);
  const auto user_entry = getpwnam(username.c_str());
  std::string dirname;

  if (!user_entry)
  {
    dirname = filename;
  }
  else
  {
    dirname = glue_prefix_and_suffix(user_entry->pw_dir, filename.c_str(), user_len);
  }

  endpwent();
  return dirname;
}

/* Return a new string which is the result of tilde expanding STRING. */
std::string tilde_expand(const std::string& input)
{
  std::string result;
  const char* string = input.c_str();

  /* Scan through STRING expanding tildes as we come to them. */
  while (true)
  {
    int len = 0;
    const auto start = tilde_find_prefix(string, &len);

    result.append(string, start);
    string += start;

    const auto end = tilde_find_suffix(string);

    /* If both START and END are zero, we are all done. */
    if (!start && !end)
    {
      break;
    }

    const std::string tilde_word(string, end);
    string += end;

    auto expansion = tilde_expand_word(tilde_word);
#ifdef __CYGWIN__
    /* Fix for Cygwin to prevent ~user/xxx from expanding to //xxx when
       $HOME for `user' is /.  On cygwin, // denotes a network drive. */
    if (expansion.size() > 1 || expansion.empty() || expansion[0] != '/' || *string != '/')
#endif
    {
      result += expansion;
    }
  }

  return result;
}
