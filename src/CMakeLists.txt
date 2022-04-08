add_subdirectory(parser)

# load package helper for generating cmake CONFIG packages
include (CMakePackageConfigHelpers)

# where to install files for "find_package"
set (benesh-pkg "share/cmake/dspaces")

add_library(benesh benesh.c)
if(_redev_FOUND)
    target_sources(benesh redev_wrapper.cpp)
endif()


target_include_directories(benesh PRIVATE
                           ${MPI_CXX_INCLUDE_PATH})
target_compile_options(benesh PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(benesh PRIVATE
                      ${MPI_CXX_LIBRARIES} 
                      ${MPI_CDX_LINK_FLAGS}
                      parser
                      dspaces::dspaces
                      ekt
                      argobots)
if(_redev_FOUND)
    target_link_libraries(benesh PRIVATE redev::redev adios2::adios2)
endif()
   
set_property(TARGET benesh PROPERTY CXX_STANDARD 20)

target_include_directories (benesh BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)

if(_apex_FOUND)
    add_definitions(-DUSE_APEX)
    target_link_libraries(benesh PRIVATE apex)
    set(NEED_APEX "apex")
endif()

set (DEST_DIR "${CMAKE_INSTALL_PREFIX}")
configure_file ("benesh.pc.in" "benesh.pc" @ONLY)

install(TARGETS benesh 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install (FILES "${CMAKE_CURRENT_BINARY_DIR}/benesh.pc"
    DESTINATION "lib/pkgconfig/")
