include(CheckCXXSourceCompiles)
include(FindPkgConfig)

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
option(
  WITH_GTK
  "Build the optional ytree-gtk GTKmm frontend."
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

set(
  YTREE_CORE_SOURCES
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
)

set(
  YTREE_UI_SHARED_SOURCES
  ui/app_common.cpp
  ui/backend_registry.cpp
  ui/dir_controller.cpp
  ui/file_controller.cpp
  ui/global.cpp
  ui/view_info.cpp
  ui/visual.cpp
  ui/compat.cpp
  ui/help.cpp
  ui/format.cpp
)

set(
  YTREE_CURSES_SOURCES
  ui/curses/clock.cpp
  ui/curses/color.cpp
  ui/curses/dir_view.cpp
  ui/curses/disp.cpp
  ui/curses/error.cpp
  ui/curses/file_view.cpp
  ui/curses/global.cpp
  ui/curses/history.cpp
  ui/curses/init.cpp
  ui/curses/input.cpp
  ui/curses/keyhtab.cpp
  ui/curses/main.cpp
  ui/curses/mouse.cpp
  ui/curses/ncurses_backend.cpp
  ui/curses/print.cpp
  ui/curses/sort.cpp
  ui/curses/stat.cpp
  ui/curses/view.cpp
)

add_library(ytree_core STATIC ${YTREE_CORE_SOURCES})
add_library(ytree_ui_shared STATIC ${YTREE_UI_SHARED_SOURCES})

foreach(lib ytree_core ytree_ui_shared)
  target_compile_features(${lib} PUBLIC cxx_std_20)
  target_compile_options(
    ${lib}
    PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
  )
  target_include_directories(
    ${lib}
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
endforeach()

target_link_libraries(
  ytree_core
  PUBLIC
    PeeloXdg
    tomlplusplus::tomlplusplus
)

target_link_libraries(
  ytree_ui_shared
  PUBLIC
    ytree_core
)

add_executable(ytree ${YTREE_CURSES_SOURCES})
target_compile_features(ytree PRIVATE cxx_std_20)
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
    ytree_ui_shared
    ${CURSES_LIBRARIES}
)

install(
  TARGETS
    ytree
  RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

if(WITH_GTK)
  pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)
  add_executable(
    ytree-gtk
    ui/gtk/main.cpp
    ui/gtk/gtk_backend.cpp
    ui/gtk/gtk_dir_view.cpp
    ui/gtk/gtk_file_view.cpp
    ui/gtk/gtk_viewer_surface.cpp
    ui/gtk/main_window.cpp
  )
  target_compile_features(ytree-gtk PRIVATE cxx_std_20)
  target_compile_options(
    ytree-gtk
    PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Werror>
  )
  target_include_directories(
    ytree-gtk
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(
    ytree-gtk
    PRIVATE
      ytree_ui_shared
      PkgConfig::GTKMM
  )
  install(
    TARGETS
      ytree-gtk
    RUNTIME DESTINATION
      ${CMAKE_INSTALL_BINDIR}
  )
endif()
