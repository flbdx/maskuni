cmake_minimum_required(VERSION 3.0)
enable_language(C)
enable_language(CXX)
include(CheckSymbolExists)
include(GNUInstallDirs)

project(maskgen)
set(MASKGEN_VERSION_MAJOR "0")
set(MASKGEN_VERSION_MINOR "3")
set(MASKGEN_VERSION_PATCH "0")
set(MASKGEN_VERSION_STRING "${MASKGEN_VERSION_MAJOR}.${MASKGEN_VERSION_MINOR}.${MASKGEN_VERSION_PATCH}")

set (MASKGEN_SOURCES
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
  set_target_properties(maskgen PROPERTIES LINK_FLAGS_RELEASE -static)
  if (WIN32)
    target_compile_options(maskgen PRIVATE -municode)
    set_target_properties(maskgen PROPERTIES LINK_FLAGS -municode)
  endif()
endif()

install(TARGETS maskgen RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_SOURCE_DIR}/README.md" DESTINATION ${CMAKE_INSTALL_DOCDIR})


set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VERSION "${MASKGEN_VERSION_STRING}")
set(CPACK_PACKAGE_VERSION_MAJOR "${MASKGEN_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${MASKGEN_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${MASKGEN_VERSION_PATCH}")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_SOURCE_DIR}/build/;${CMAKE_SOURCE_DIR}/.git/;.*~;.*kdev4;.gitignore")

include(CPack)

