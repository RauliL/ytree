#pragma once

#include <string>

/* Return a new string which is the result of tilde expanding STRING. */
std::string tilde_expand(const std::string& string);

/* Do the work of tilde expansion on FILENAME.  FILENAME starts with a
   tilde. */
std::string tilde_expand_word(const std::string& filename);
