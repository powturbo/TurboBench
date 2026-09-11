cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

# Iguana Project
# ==============

project(iguana CXX)

set(IGUANA_SRC
  "iguana/ans1.cpp"
  "iguana/ans1.h"
  "iguana/ans32.cpp"
  "iguana/ans32_avx512.cpp"
  "iguana/ans32.h"
  "iguana/ans_bitstream.cpp"
  "iguana/ans_bitstream.h"
  "iguana/ans_byte_statistics.cpp"
  "iguana/ans_byte_statistics.h"
  "iguana/ans_decoder.h"
  "iguana/ans_encoder.h"
  "iguana/ans_nibble.cpp"
  "iguana/ans_nibble.h"
  "iguana/ans_nibble_statistics.cpp"
  "iguana/ans_nibble_statistics.h"
  "iguana/bitops.h"
#  "iguana/c_bindings.cpp"
#  "iguana/c_bindings.h"
  "iguana/command.h"
  "iguana/common.cpp"
  "iguana/common.h"
  "iguana/decoder.cpp"
  "iguana/decoder.h"
  "iguana/encoder.cpp"
  "iguana/encoder.h"
  "iguana/entropy.cpp"
  "iguana/entropy.h"
  "iguana/error.cpp"
  "iguana/error.h"
#  "iguana/file.cpp"
#  "iguana/file.h"
  "iguana/input_stream.h"
  "iguana/memops.h"
  "iguana/output_stream.cpp"
  "iguana/output_stream.h"
  "iguana/platform.h"
  "iguana/span.h"
  "iguana/utils.h"
#  "main.cpp"
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${IGUANA_SRC})

if (NOT DEFINED IGUANA_TEST)
  set(IGUANA_TEST FALSE)
endif()

if (NOT DEFINED IGUANA_STATIC)
  set(IGUANA_STATIC FALSE)
endif()

if (NOT DEFINED IGUANA_SANITIZE)
  set(IGUANA_SANITIZE FALSE)
endif()

set(IGUANA_TEST
    "${IGUANA_TEST}"
    CACHE BOOL "Build Iguana tests")

set(IGUANA_STATIC
    "${IGUANA_STATIC}"
    CACHE BOOL "Build Iguana as a static library")

set(IGUANA_SANITIZE
    "${IGUANA_SANITIZE}"
    CACHE STRING "Build Iguana with sanitizers: 'address', 'undefined', etc...")

if (IGUANA_STATIC)
  set(IGUANA_LIBRARY_TYPE "STATIC")
else()
  set(IGUANA_LIBRARY_TYPE "SHARED")
endif()

# Iguana CMake Boilerplate
# ========================

cmake_policy(PUSH)

if (POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW) # Honor visibility properties.
endif()

if (POLICY CMP0092)
  cmake_policy(SET CMP0092 NEW) # Don't add -W3 warning level by default.
endif()

include(CheckCXXCompilerFlag)
include(GNUInstallDirs)

# Detects C++ compile flags.
function(iguana_detect_cflags out)
  set(_out_array ${${out}})
  string(REGEX REPLACE "[+]" "x" _flag_signature "${ARGN}")
  string(REGEX REPLACE "[-= :;/.\]" "_" _flag_signature "${_flag_signature}")

  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(GNU|Clang|AppleClang)$")
    check_cxx_compiler_flag("${ARGN} -Werror" "__CxxFlag_${_flag_signature}")
  else()
    check_cxx_compiler_flag("${ARGN}" "__CxxFlag_${_flag_signature}")
  endif()

  if (${__CxxFlag_${_flag_signature}})
    list(APPEND _out_array ${ARGN})
  endif()
  set(${out} "${_out_array}" PARENT_SCOPE)
endfunction()

# Detects C++ sanitizers.
function(iguana_detect_sanitizers out)
  set(_out_array ${${out}})
  set(_flags "")

  foreach(_arg ${ARGN})
    string(REPLACE "," ";" _arg "${_arg}")
    list(APPEND _flags ${_arg})
  endforeach()

  foreach(_flag ${_flags})
    if (NOT "${_flag}" MATCHES "^-fsanitize=")
      SET(_flag "-fsanitize=${_flag}")
    endif()

    # Sanitizers also require link flags, see CMAKE_REQUIRED_FLAGS.
    set(CMAKE_REQUIRED_FLAGS "${_flag}")
    iguana_detect_cflags(_out_array ${_flag})
    unset(CMAKE_REQUIRED_FLAGS)
  endforeach()

  set(${out} "${_out_array}" PARENT_SCOPE)
endfunction()

# Convenience function to add a target to the project.
function(iguana_add_target target target_type)
  set(single_val "")
  set(multi_val SOURCES LIBRARIES PUBLIC_CFLAGS PRIVATE_CFLAGS)
  cmake_parse_arguments("X" "" "${single_val}" "${multi_val}" ${ARGN})

  if ("${target_type}" MATCHES "^(EXECUTABLE|TEST)$")
    add_executable(${target} ${X_SOURCES})
  else()
    add_library(${target} ${target_type} ${X_SOURCES})
  endif()

  set_target_properties(${target} PROPERTIES DEFINE_SYMBOL "")
  target_link_libraries(${target} PRIVATE ${X_LIBRARIES})

  # target_link_options was added in cmake v3.13, don't use it for now...
  foreach(link_flag ${IGUANA_SANITIZE_LFLAGS})
    set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${link_flag}")
  endforeach()

  target_compile_features(${target} PUBLIC cxx_std_17)
  set_property(TARGET ${target} PROPERTY CXX_EXTENSIONS NO)
  set_property(TARGET ${target} PROPERTY CXX_VISIBILITY_PRESET hidden)
  target_compile_options(${target} PRIVATE ${X_PRIVATE_CFLAGS} ${IGUANA_SANITIZE_CFLAGS})
  target_compile_options(${target} PUBLIC ${X_PUBLIC_CFLAGS})

  if ("${target_type}" STREQUAL "TEST")
    add_test(NAME ${target} COMMAND ${target})
  endif()
endfunction()

# Iguana Build Setup
# ==================

set(IGUANA_LIBRARIES "")
set(IGUANA_PUBLIC_CFLAGS "")
set(IGUANA_PRIVATE_CFLAGS "-DIGUANA_EXPORTS=1")
set(IGUANA_SANITIZE_CFLAGS "")
set(IGUANA_SANITIZE_LFLAGS "")

if (IGUANA_STATIC)
  list(APPEND IGUANA_PUBLIC_CFLAGS "-DIGUANA_STATIC")
endif()

# Baseline Iguana build configuration.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "x${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "xMSVC")
  list(APPEND IGUANA_PRIVATE_CFLAGS
    -GF                      # [+] Eliminate duplicate strings.
    -Zc:__cplusplus          # [+] Conforming __cplusplus definition.
    -Zc:inline               # [+] Remove unreferenced COMDAT.
    -Zc:strictStrings        # [+] Strict const qualification of string literals.
    -W4		                 # [+] Warning level 4.
    -std:c++20)              # [+] Enable the C++20 features
    add_compile_definitions(IGUANA_COMPILER_MSVC=true)
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(GNU|Clang|AppleClang)$")
  list(APPEND IGUANA_PRIVATE_CFLAGS -Wall -Wextra -Wconversion)
  list(APPEND IGUANA_PRIVATE_CFLAGS -fno-math-errno)
endif()

# Sanitizer-enabled Iguana build configuration.
if (IGUANA_SANITIZE)
  iguana_detect_sanitizers(IGUANA_SANITIZE_CFLAGS ${IGUANA_SANITIZE})
  if (IGUANA_SANITIZE_CFLAGS)
    message("-- Enabling sanitizers for Iguana build: '${IGUANA_SANITIZE_CFLAGS}'")

    set(IGUANA_SANITIZE_LFLAGS ${IGUANA_SANITIZE_CFLAGS})

    # Don't omit frame pointer if sanitizers are enabled.
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "x${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "xMSVC")
      list(APPEND IGUANA_SANITIZE_CFLAGS -Oy-)
    else()
      list(APPEND IGUANA_SANITIZE_CFLAGS -fno-omit-frame-pointer -g)
    endif()

    list(APPEND IGUANA_PRIVATE_CFLAGS ${IGUANA_SANITIZE_CFLAGS})
    list(APPEND IGUANA_PRIVATE_LFLAGS ${IGUANA_SANITIZE_LFLAGS})
  endif()
endif()

# Do not link to pthread on Android as it's part of C runtime.
if (NOT WIN32 AND NOT ANDROID)
  list(APPEND IGUANA_LIBRARIES pthread)
endif()

# Iguana Build Targets
# ====================

iguana_add_target(iguana "${IGUANA_LIBRARY_TYPE}"
                  SOURCES ${IGUANA_SRC}
                  LIBRARIES ${IGUANA_LIBRARIES}
                  PUBLIC_CFLAGS ${IGUANA_PUBLIC_CFLAGS}
                  PRIVATE_CFLAGS ${IGUANA_PRIVATE_CFLAGS})

target_include_directories(iguana BEFORE INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Iguana library alias to be used by cmake (always used as a namespace::name pair).
add_library(iguana::iguana ALIAS iguana)

if (IGUANA_TEST)
  enable_testing()

  # TODO: Test application(s).
endif()

# Iguana Install Instructions
# ===========================

# Install Iguana header files, but not header files ending with _p.h (private headers).
foreach(_src_file ${IGUANA_SRC})
  if ("${_src_file}" MATCHES "\\.h$" AND NOT "${_src_file}" MATCHES "_p\\.h$")
    get_filename_component(_src_dir ${_src_file} PATH)
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/${_src_file}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${_src_dir}")
  endif()
endforeach()

# Install Iguana cmake scripts (so Iguana can be found by a build system when using it).
install(TARGETS iguana
        EXPORT iguana-config
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT iguana-config
        NAMESPACE iguana::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/iguana")

cmake_policy(POP)
