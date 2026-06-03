include(FetchContent)

FetchContent_Declare(
  avendish
  GIT_REPOSITORY "https://github.com/celtera/avendish"
  GIT_TAG  f26e07e04f88261fb96b6538f9c1e7b7940b473c
  GIT_PROGRESS true
)
FetchContent_Populate(avendish)

set(CMAKE_PREFIX_PATH "${avendish_SOURCE_DIR};${CMAKE_PREFIX_PATH}")
find_package(Avendish REQUIRED)
