cmake_minimum_required(VERSION 2.8)

project(brieflz C)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TEST "Build unit test" ON)
option(BUILD_COVERAGE "Code coverage" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS)
  if(NOT MSVC_VERSION LESS 1700)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /analyze")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC)
  set(gcc_warning_flags
    -Wall
    -Wextra
    -pedantic
    -Wshadow
    -Wpointer-arith
    -Wcast-qual
    -Wcast-align
    -Wstrict-prototypes
    -Wmissing-prototypes
  )
  foreach(flag ${gcc_warning_flags})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
  endforeach()
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Weverything -pedantic")
endif()

if(BUILD_COVERAGE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
endif()

include_directories(${PROJECT_SOURCE_DIR})

add_library(brieflz brieflz.c depack.c depacks.c brieflz.h)
set_target_properties(brieflz PROPERTIES
  VERSION 1.1.0
  SOVERSION 1
  C_VISIBILITY_PRESET hidden
)

if(NOT CMAKE_VERSION VERSION_LESS 2.8.11)
  target_compile_definitions(brieflz
    PRIVATE $<$<STREQUAL:$<TARGET_PROPERTY:brieflz,TYPE>,SHARED_LIBRARY>:BLZ_DLL_EXPORTS>
    PUBLIC $<$<STREQUAL:$<TARGET_PROPERTY:brieflz,TYPE>,SHARED_LIBRARY>:BLZ_DLL>
  )
endif()

add_executable(blzpack example/blzpack.c example/parg.c example/parg.h)
target_link_libraries(blzpack brieflz)

if(BUILD_TEST)
  add_executable(test_brieflz test/test_brieflz.c test/greatest.h brieflz.h)
  target_link_libraries(test_brieflz brieflz)

  enable_testing()
  add_test(test_brieflz test_brieflz)
endif()

install(TARGETS brieflz blzpack
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(FILES brieflz.h
  DESTINATION include
)
