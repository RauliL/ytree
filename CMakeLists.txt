CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(YTree LANGUAGES C)

SET(CURSES_NEED_NCURSES TRUE)
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

INCLUDE(FindCurses)
INCLUDE(FindReadline)

ADD_DEFINITIONS(
  -DCOLOR_SUPPORT
#  -DCLOCK_SUPPORT # Experimental!
  -DREADLINE_SUPPORT
  -DWITH_UTF8
)

ADD_EXECUTABLE(
  ytree
  arc.c
  archive.c
  chgrp.c
  chmod.c
  chown.c
  clock.c
  color.c
  copy.c
  delete.c
  dirwin.c
  disp.c
  edit.c
  error.c
  execute.c
  filespec.c
  filewin.c
  freesp.c
  global.c
  group.c
  hex.c
  history.c
  init.c
  input.c
  keyhtab.c
  lha.c
  login.c
  main.c
  match.c
  mkdir.c
  mktime.c
  move.c
  passwd.c
  pipe.c
  print.c
  profile.c
  quit.c
  rar.c
  readtree.c
  rename.c
  rmdir.c
  rpm.c
  sort.c
  stat.c
  system.c
  tar.c
  termcap.c
  tilde.c
  usermode.c
  util.c
  view.c
  xmalloc.c
  zip.c
  zoo.c
)

TARGET_INCLUDE_DIRECTORIES(
  ytree
  PRIVATE
    $<BUILD_INTERFACE:${CURSES_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${READLINE_INCLUDE_DIRS}>
)

TARGET_LINK_LIBRARIES(
  ytree
  PRIVATE
    ${CURSES_LIBRARIES}
    ${READLINE_LIBRARIES}
)

INSTALL(
  TARGETS
    ytree
  RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
