SET(CURSES_NEED_NCURSES TRUE)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")

INCLUDE(FindCurses)
INCLUDE(FindReadline)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h
)

ADD_DEFINITIONS(
  -DCOLOR_SUPPORT
#  -DCLOCK_SUPPORT # Experimental!
  -DREADLINE_SUPPORT
  -DWITH_UTF8
)

ADD_EXECUTABLE(
  ytree
  arc.cpp
  archive.cpp
  chgrp.cpp
  chmod.cpp
  chown.cpp
  clock.cpp
  color.cpp
  copy.cpp
  delete.cpp
  dirwin.cpp
  disp.cpp
  edit.cpp
  error.cpp
  execute.cpp
  filespec.cpp
  filewin.cpp
  freesp.cpp
  global.cpp
  group.cpp
  hex.cpp
  history.cpp
  init.cpp
  input.cpp
  keyhtab.cpp
  lha.cpp
  login.cpp
  main.cpp
  match.cpp
  mkdir.cpp
  mktime.cpp
  move.cpp
  passwd.cpp
  pipe.cpp
  print.cpp
  profile.cpp
  quit.cpp
  rar.cpp
  readtree.cpp
  rename.cpp
  rmdir.cpp
  rpm.cpp
  sort.cpp
  stat.cpp
  system.cpp
  tar.cpp
  tilde.cpp
  usermode.cpp
  util.cpp
  view.cpp
  xmalloc.cpp
  zip.cpp
  zoo.cpp
)

TARGET_COMPILE_FEATURES(
  ytree
  PRIVATE
    cxx_std_17
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
