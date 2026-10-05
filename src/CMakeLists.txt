LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")

OPTION(
  COLOR_SUPPORT
  "Whether colors should be enabled or not."
  ON
)
OPTION(
  CLOCK_SUPPORT
  "Whether experimental clock should be enabled or not."
  OFF
)
OPTION(
  WITH_UTF8
  "Whether support for UTF-8 should be enabled or not."
  ON
)

# auto: PDCursesMod on Windows, ncurses elsewhere
SET(
  YTREE_CURSES_BACKEND
  "auto"
  CACHE STRING
  "Curses backend: auto, ncurses, or pdcurses"
)
SET_PROPERTY(
  CACHE YTREE_CURSES_BACKEND
  PROPERTY STRINGS auto ncurses pdcurses
)

IF(YTREE_CURSES_BACKEND STREQUAL "auto")
  IF(WIN32)
    SET(YTREE_CURSES_BACKEND_EFFECTIVE "pdcurses")
  ELSE()
    SET(YTREE_CURSES_BACKEND_EFFECTIVE "ncurses")
  ENDIF()
ELSE()
  SET(YTREE_CURSES_BACKEND_EFFECTIVE "${YTREE_CURSES_BACKEND}")
ENDIF()

MESSAGE(STATUS "ytree curses backend: ${YTREE_CURSES_BACKEND_EFFECTIVE}")

IF(YTREE_CURSES_BACKEND_EFFECTIVE STREQUAL "pdcurses")
  INCLUDE(PDCursesMod)
  SET(YTREE_CURSES_PDCURSES TRUE)
ELSEIF(YTREE_CURSES_BACKEND_EFFECTIVE STREQUAL "ncurses")
  SET(CURSES_NEED_NCURSES TRUE)
  IF(WITH_UTF8)
    SET(CURSES_NEED_WIDE TRUE)
  ENDIF()

  INCLUDE(FindCurses)

  IF(NOT CURSES_FOUND)
    IF(WITH_UTF8)
      MESSAGE(FATAL_ERROR "wide ncurses development package is required (e.g. libncurses-dev / libncursesw-dev)")
    ELSE()
      MESSAGE(FATAL_ERROR "ncurses development package is required (e.g. libncurses-dev)")
    ENDIF()
  ENDIF()
ELSE()
  MESSAGE(FATAL_ERROR "Unknown YTREE_CURSES_BACKEND='${YTREE_CURSES_BACKEND}' (use auto, ncurses, or pdcurses)")
ENDIF()

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h
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
  complete.cpp
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
  path.cpp
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
  zip.cpp
  zoo.cpp
)

TARGET_COMPILE_FEATURES(
  ytree
  PRIVATE
    cxx_std_20
)

IF(YTREE_CURSES_BACKEND_EFFECTIVE STREQUAL "pdcurses")
  TARGET_LINK_LIBRARIES(
    ytree
    PRIVATE
      ${YTREE_PDCURSES_TARGET}
      PeeloXdg
      tomlplusplus::tomlplusplus
  )
ELSE()
  TARGET_INCLUDE_DIRECTORIES(
    ytree
    PRIVATE
      $<BUILD_INTERFACE:${CURSES_INCLUDE_DIR}>
  )
  TARGET_LINK_LIBRARIES(
    ytree
    PRIVATE
      ${CURSES_LIBRARIES}
      PeeloXdg
      tomlplusplus::tomlplusplus
  )
ENDIF()

INSTALL(
  TARGETS
    ytree
  RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
