include(CheckCXXSourceCompiles)

option(
  COLOR_SUPPORT
  "Whether colors should be enabled or not."
  ON
)
option(
  CLOCK_SUPPORT
  "Whether experimental clock should be enabled or not."
  OFF
)
option(
  WITH_UTF8
  "Whether support for UTF-8 should be enabled or not."
  ON
)
option(
  VI_KEYS
  "Whether Vi keys should be enabled or not."
  OFF
)

set(
  COLOR_THEME
  "gruvbox"
  CACHE STRING
  "Color theme when COLOR_SUPPORT is ON (original or gruvbox)."
)
set_property(
  CACHE
    COLOR_THEME
  PROPERTY
    STRINGS
    original
    gruvbox
)
string(TOLOWER "${COLOR_THEME}" COLOR_THEME_LOWER)
if(COLOR_THEME_LOWER STREQUAL "gruvbox")
  set(COLOR_THEME_GRUVBOX TRUE)
elseif(COLOR_THEME_LOWER STREQUAL "original")
  set(COLOR_THEME_GRUVBOX FALSE)
else()
  message(
    FATAL_ERROR
    "COLOR_THEME must be 'original' or 'gruvbox' (got '${COLOR_THEME}')"
  )
endif()

set(CURSES_NEED_NCURSES TRUE)
if(WITH_UTF8)
  set(CURSES_NEED_WIDE TRUE)
endif()

include(FindCurses)

if(NOT CURSES_FOUND)
  if(WITH_UTF8)
    message(
      FATAL_ERROR
      "wide ncurses development package is required (e.g. libncurses-dev / libncursesw-dev)"
    )
  else()
    message(
      FATAL_ERROR
      "ncurses development package is required (e.g. libncurses-dev)"
    )
  endif()
endif()

check_cxx_source_compiles("
#include <chrono>
int main()
{
  const auto local = std::chrono::zoned_time{
    std::chrono::current_zone(),
    std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())
  };
  (void)local;
  return 0;
}
" YTREE_HAS_STD_CHRONO_TZ)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp
)

add_executable(
  ytree

  archive/arc.cpp
  archive/lha.cpp
  archive/rar.cpp
  archive/rpm.cpp
  archive/sevenzip.cpp
  archive/tar.cpp
  archive/zip.cpp
  archive/zoo.cpp

  core/archive.cpp
  core/chgrp.cpp
  core/chmod.cpp
  core/chown.cpp
  core/copy.cpp
  core/delete.cpp
  core/edit.cpp
  core/entry_meta.cpp
  core/execute.cpp
  core/filespec.cpp
  core/freesp.cpp
  core/group.cpp
  core/hex.cpp
  core/login.cpp
  core/match.cpp
  core/mkdir.cpp
  core/mktime.cpp
  core/move.cpp
  core/passwd.cpp
  core/path.cpp
  core/pipe.cpp
  core/profile.cpp
  core/quit.cpp
  core/readtree.cpp
  core/rename.cpp
  core/rmdir.cpp
  core/system.cpp
  core/tilde.cpp
  core/usermode.cpp
  core/util.cpp

  ui/clock.cpp
  ui/color.cpp
  ui/dirwin.cpp
  ui/disp.cpp
  ui/error.cpp
  ui/filewin.cpp
  ui/global.cpp
  ui/history.cpp
  ui/init.cpp
  ui/input.cpp
  ui/keyhtab.cpp
  ui/main.cpp
  ui/mouse.cpp
  ui/print.cpp
  ui/sort.cpp
  ui/stat.cpp
  ui/view.cpp
)

target_compile_features(
  ytree
  PRIVATE
    cxx_std_20
)

target_compile_options(
  ytree
  PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
)

target_include_directories(
  ytree
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<BUILD_INTERFACE:${CURSES_INCLUDE_DIR}>
)

target_link_libraries(
  ytree
  PRIVATE
    ${CURSES_LIBRARIES}
    PeeloXdg
    tomlplusplus::tomlplusplus
)

install(
  TARGETS
    ytree
  RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
