cmake_minimum_required(VERSION 3.0)
include(CheckSymbolExists)

project(maskgen)

set (MASKGEN_SOURCES
        src/ReadCharsets.cpp src/ReadMasks.cpp src/main.cpp)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # default release flags : remove -DNDEBUG (its a test project, want the asserts...) 
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  # default debug flags : add sanitizer and up the debug symbols
  set(CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address -fsanitize=leak -fsanitize=undefined")
endif()

# some ugly libc like msys2's don't have getline
check_symbol_exists(getline "stdio.h" HAVE_GETLINE)
if (NOT HAVE_GETLINE)
    list(APPEND MASKGEN_SOURCES lib/getdelim.c lib/getline.c)
endif()

configure_file(src/config.h.in src/config.h)

add_executable(maskgen ${MASKGEN_SOURCES})
target_compile_definitions(maskgen PRIVATE _GNU_SOURCE _FILE_OFFSET_BITS=64)
target_include_directories(maskgen PRIVATE lib/ src/ ${PROJECT_BINARY_DIR}/src/)
target_compile_features(maskgen PUBLIC cxx_std_11)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # additional warnings
  target_compile_options(maskgen PRIVATE -Wall -Wextra)
endif()

install(TARGETS maskgen RUNTIME DESTINATION bin)
