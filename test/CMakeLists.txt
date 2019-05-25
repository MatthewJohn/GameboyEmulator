cmake_minimum_required (VERSION 3.5)

project (GameboyEmulator.Test)

set (CMAKE_BUILD_TYPE Release)
#set (SFML_STATIC_LIBRARIES TRUE)
#set (SFML_STATIC_LIBRARIES FALSE)
set (BUILD_SHARED_LIBS)
set (CMAKE_MODULE_PATH "/usr/include/SDL2")

find_package(SDL2 REQUIRED sdl2)
#include_directories(${SFML_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
set (CMAKE_EXE_LINKER_FLAGS "-Wl,--copy-dt-needed-entries")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")
set (source_dir "${PROJECT_SOURCE_DIR}/src/")

file (GLOB source_files "${source_dir}/*.cpp")

add_executable (GameboyEmulator.Test ${source_files})
#target_link_libraries(GameboyEmulator sfml-graphics sfml-audio)
target_link_libraries(GameboyEmulator.Test ${SDL2_LIBRARIES})
