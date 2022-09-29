cmake_minimum_required (VERSION 3.9)
project (benesh VERSION 0.2.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CTest)

cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0088 NEW)

if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif()

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# link shared lib with full rpath
set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set (CMAKE_PREFIX_PATH "" CACHE STRING "External dependencies path")
#set (BUILD_SHARED_LIBS "OFF" CACHE BOOL "Build a shared library")

option(ENABLE_TIMING "enable APEX timing output" OFF)

include (xpkg-import)
xpkg_import_module(ekt REQUIRED ekt)
xpkg_import_module(argobots REQUIRED argobots)
if(${ENABLE_TIMING})
  xpkg_import_module(apex apex)
endif()

if(ENABLE_DSP_REDEV)
    add_definitions(-DUSE_DSP_REDEV)
endif()

find_package(dspaces REQUIRED)
find_package(MPI COMPONENTS REQUIRED)
find_package(redev REQUIRED)
find_package(ADIOS2 2.7.1 COMPONENTS REQUIRED)
find_package(Omega_h REQUIRED VERSION 10)
find_package(wdmcpl REQUIRED)
message(STATUS "Found Omega_h: ${Omega_h_DIR} (found version ${Omega_h_VERSION})")
if(NOT Omega_h_USE_MPI)
  message(FATAL_ERROR "Omega_h must be built with MPI enabled.")
endif()

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)
