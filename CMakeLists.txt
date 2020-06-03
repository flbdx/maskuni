cmake_minimum_required(VERSION 3.0)
enable_language(C)
enable_language(CXX)
include(CheckSymbolExists)
include(GNUInstallDirs)

project(maskuni)
set(MASKUNI_VERSION_MAJOR "0")
set(MASKUNI_VERSION_MINOR "3")
set(MASKUNI_VERSION_PATCH "1")
set(MASKUNI_VERSION_STRING "${MASKUNI_VERSION_MAJOR}.${MASKUNI_VERSION_MINOR}.${MASKUNI_VERSION_PATCH}")

set (MASKUNI_SOURCES
        src/ReadCharsets.cpp src/ReadMasks.cpp src/ReadBruteforce.cpp src/main.cpp)

# sanitize default release and debug flags for clang and gcc
# don't want -DNDEBUG on release
# add full debug symbols, ASAN and UBSAN on debug
if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_C_FLAGS_RELEASE "-O3")
  set(CMAKE_C_FLAGS_DEBUG "-g3 -fsanitize=address -fsanitize=leak -fsanitize=undefined")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g3")
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  set(CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address -fsanitize=leak -fsanitize=undefined")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g3")
endif()

# some ugly libc like msys2's don't have getline even though getline is part of posix since POSIX.1-2008
check_symbol_exists(getline "stdio.h" HAVE_GETLINE)
if (NOT HAVE_GETLINE)
    list(APPEND MASKUNI_SOURCES lib/getdelim.c lib/getline.c)
endif()

configure_file(src/config.h.in src/config.h)

add_executable(maskuni ${MASKUNI_SOURCES})
target_compile_definitions(maskuni PRIVATE _GNU_SOURCE _FILE_OFFSET_BITS=64)
target_include_directories(maskuni PRIVATE lib/ src/ ${PROJECT_BINARY_DIR}/src/)
target_compile_features(maskuni PUBLIC cxx_std_11)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # additional warnings
  target_compile_options(maskuni PRIVATE -Wall -Wextra)
  set_target_properties(maskuni PROPERTIES LINK_FLAGS_RELEASE -static)
  if (WIN32)
    target_compile_options(maskuni PRIVATE -municode)
    set_target_properties(maskuni PROPERTIES LINK_FLAGS -municode)
  endif()
endif()

install(TARGETS maskuni RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_SOURCE_DIR}/README.md" DESTINATION ${CMAKE_INSTALL_DOCDIR})


set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VERSION "${MASKUNI_VERSION_STRING}")
set(CPACK_PACKAGE_VERSION_MAJOR "${MASKUNI_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${MASKUNI_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${MASKUNI_VERSION_PATCH}")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_SOURCE_DIR}/build/;${CMAKE_SOURCE_DIR}/.git/;.*~;.*kdev4;.gitignore")

include(CPack)

