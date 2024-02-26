cmake_minimum_required (VERSION 3.5)

project (GameboyEmulator)

set (CMAKE_BUILD_TYPE Release)
set (BUILD_SHARED_LIBS)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")

# Adjust linker flags based on platform
if(UNIX AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,--copy-dt-needed-entries")
endif()

set (source_dir "${PROJECT_SOURCE_DIR}/src/")
file (GLOB source_files "${source_dir}/*.cpp")
add_executable (GameboyEmulator ${source_files})
target_link_libraries(GameboyEmulator ${SDL2_LIBRARIES})
