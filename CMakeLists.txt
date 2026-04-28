cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(verihogg-format VERSION 1.0 LANGUAGES CXX)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
endif()

# Use C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(slang REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Core library — everything except main.cpp
file(GLOB_RECURSE CORE_SRC formatter/src/**.cpp)
list(FILTER CORE_SRC EXCLUDE REGEX ".*/main\\.cpp$")

add_library(formatter_core STATIC ${CORE_SRC})

target_include_directories(formatter_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/formatter/include>
)

target_link_libraries(formatter_core PUBLIC slang::slang Microsoft.GSL::GSL)

if(MSVC)
    target_compile_options(formatter_core PRIVATE /W4 /bigobj)
else()
    target_compile_options(formatter_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Main executable
add_executable(${PROJECT_NAME} formatter/src/main.cpp)
target_link_libraries(${PROJECT_NAME} formatter_core)

set(GTEST_ROOT "" CACHE PATH "Path to GTest installation prefix")

find_path(GTEST_INCLUDE_DIR
  NAMES gtest/gtest.h
  HINTS ${GTEST_ROOT} ENV GTEST_ROOT
  PATH_SUFFIXES include
)

find_library(GTEST_LIBRARY
  NAMES googletest gtest
  HINTS ${GTEST_ROOT} ENV GTEST_ROOT
  PATH_SUFFIXES lib lib64
)

find_library(GTEST_MAIN_LIBRARY
  NAMES gtest_main
  HINTS ${GTEST_ROOT} ENV GTEST_ROOT
  PATH_SUFFIXES lib lib64
)

if(NOT GTEST_INCLUDE_DIR OR NOT GTEST_LIBRARY)
  message(FATAL_ERROR
    "GTest was not found. Install GTest and set GTEST_ROOT (or pass -DGTEST_ROOT=<prefix>).")
endif()

enable_testing()
add_subdirectory(tests)

# Install rule
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

