#pragma once

#include <string>

/* Build a readline-style match list for filename completion.
 *
 * matches[0] is the longest common prefix (the substitution text).
 * matches[1..] are the individual matches.
 * The array is terminated with a nullptr.
 *
 * Returns nullptr when there are no matches. The result must be released
 * with free_completion_matches(). */
char** filename_completion_matches(const std::string& text);

/* Free a list returned by filename_completion_matches(). */
void free_completion_matches(char** matches);
