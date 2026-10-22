cmake_minimum_required(VERSION 3.32)

project(YTree LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
fetchcontent_declare(
  PeeloXdg
  GIT_REPOSITORY
    https://github.com/peelonet/peelo-xdg.git
  GIT_TAG
    v1.2.1
)
fetchcontent_makeavailable(PeeloXdg)

fetchcontent_declare(
  tomlplusplus
  GIT_REPOSITORY
    https://github.com/marzer/tomlplusplus.git
  GIT_TAG
    v3.4.0
)
fetchcontent_makeavailable(tomlplusplus)

fetchcontent_declare(
  libterm
  GIT_REPOSITORY
    https://github.com/rizukirr/libterm.git
  GIT_TAG
    v0.1.2
)
fetchcontent_makeavailable(libterm)

add_subdirectory(man)
add_subdirectory(share)
add_subdirectory(src)
