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

include(CMakeDependentOption)
get_property(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
cmake_dependent_option(BUILD_SHARED_LIBS
  "Build shared libraries (so/dylib/dll)." ${SHARED_LIBS_SUPPORTED}
  "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(BUILD_SHARED_LIBS)
if((NOT BUILD_SHARED_LIBS) AND (NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE))
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set (BUILD_SHARED_LIBS "OFF" CACHE BOOL "Build a shared library")

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
find_package(Kokkos REQUIRED)
message(STATUS "Found Omega_h: ${Omega_h_DIR} (found version ${Omega_h_VERSION})")
if(NOT Omega_h_USE_MPI)
  message(FATAL_ERROR "Omega_h must be built with MPI enabled.")
endif()

include(CheckLanguage)
check_language(Fortran)
if(CMAKE_Fortran_COMPILER)
    enable_language(Fortran)
endif()
if(CMAKE_Fortran_COMPILER_LOADED)
    include(CheckFortranCompilerFlag)
    set(BENESH_HAVE_FORTRAN TRUE)
    check_fortran_compiler_flag("-fallow-argument-mismatch" FORTRAN_HAS_ARG_MISMATCH_FLAG)
    if(FORTRAN_HAS_ARG_MISMATCH_FLAG)
        SET(CMAKE_Fortran_FLAGS "${CMAKE_CXX_FLAGS} -fallow-argument-mismatch")
    endif()
endif()

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(bindings)
add_subdirectory(tests)
