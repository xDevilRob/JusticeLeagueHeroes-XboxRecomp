cmake_minimum_required(VERSION 3.25)
project(JusticeLeagueHeroes LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include("${CMAKE_CURRENT_LIST_DIR}/../sdk/cmake/XbeRecomp.cmake")
file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/common/*.cpp)
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/core/log[.]cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/xdk/d3d8[.]cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/xdk/dsound[.]cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/gpu/shaders[.]cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/kernel/xboxkrnl[.]cpp$")
add_xbe_image(JusticeLeagueHeroes default.xbe game.toml 32 justice_league_heroes)
# Match the official template's CPU baseline (includes SSE4.1).
target_compile_options(JusticeLeagueHeroes PRIVATE -march=x86-64-v2)
enable_language(RC)
configure_file(launcher_icon.rc.in "${CMAKE_CURRENT_BINARY_DIR}/launcher_icon.rc" @ONLY)
target_sources(JusticeLeagueHeroes PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/launcher_icon.rc")
# Source properties are scoped to the directory defining the game target.
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/../sdk/runtime/core/input.cpp"
    PROPERTIES INCLUDE_DIRECTORIES "${sdl3_SOURCE_DIR}/include")



