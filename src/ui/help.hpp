#pragma once

#include <array>
#include <string>

/** Two-line help banner shown at the bottom of the main UI. */
using HelpLines = std::array<std::string, 2>;

/** Mode-aware directory-window help (disk / user / archive / LL). */
HelpLines DirHelpLines();

/** Mode-aware file-window help (disk / user / archive / LL). */
HelpLines FileHelpLines();
