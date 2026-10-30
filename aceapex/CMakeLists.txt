cmake_minimum_required(VERSION 3.14)
project(aceapex CXX)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-O3 -funroll-loops)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)

add_executable(aceapex src/aceapex_main.cpp)
target_link_libraries(aceapex PRIVATE Threads::Threads PkgConfig::ZSTD)

install(TARGETS aceapex DESTINATION bin)
