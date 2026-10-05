# Fetch and build PDCursesMod as a static library for ytree.
#
# Sets:
#   YTREE_PDCURSES_TARGET  - CMake target to link
#   YTREE_PDCURSES_PORT    - port name (wincon or vt)

include(FetchContent)

set(YTREE_PDCURSES_TAG "v4.5.4" CACHE STRING "PDCursesMod git tag")

FetchContent_Declare(
  PDCursesMod
  GIT_REPOSITORY https://github.com/Bill-Gray/PDCursesMod.git
  GIT_TAG ${YTREE_PDCURSES_TAG}
  GIT_SHALLOW TRUE
)

# We compile selected port sources ourselves; do not run PDCursesMod's CMake.
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()
FetchContent_GetProperties(PDCursesMod)
if(NOT pdcursesmod_POPULATED)
  FetchContent_Populate(PDCursesMod)
endif()

if(WIN32)
  set(YTREE_PDCURSES_PORT wincon)
else()
  set(YTREE_PDCURSES_PORT vt)
endif()

file(GLOB YTREE_PDCURSES_COMMON CONFIGURE_DEPENDS
  "${pdcursesmod_SOURCE_DIR}/pdcurses/*.c"
)
file(GLOB YTREE_PDCURSES_PORT_SOURCES CONFIGURE_DEPENDS
  "${pdcursesmod_SOURCE_DIR}/${YTREE_PDCURSES_PORT}/*.c"
)

add_library(
  ytree_pdcurses
  STATIC
  ${YTREE_PDCURSES_COMMON}
  ${YTREE_PDCURSES_PORT_SOURCES}
)

set_target_properties(
  ytree_pdcurses
  PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(
  ytree_pdcurses
  PUBLIC
    "${pdcursesmod_SOURCE_DIR}"
  PRIVATE
    "${pdcursesmod_SOURCE_DIR}/${YTREE_PDCURSES_PORT}"
    "${pdcursesmod_SOURCE_DIR}/common"
)

if(WITH_UTF8)
  target_compile_definitions(
    ytree_pdcurses
    PUBLIC
      PDC_WIDE
      PDC_FORCE_UTF8
  )
endif()

if(WIN32 AND YTREE_PDCURSES_PORT STREQUAL "wincon")
  target_link_libraries(ytree_pdcurses PUBLIC winmm)
endif()

set(YTREE_PDCURSES_TARGET ytree_pdcurses)
