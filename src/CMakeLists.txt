add_subdirectory(parser)

# load package helper for generating cmake CONFIG packages
include (CMakePackageConfigHelpers)

# where to install files for "find_package"
set (benesh-pkg "share/cmake/benesh")

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
#set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")

add_library(benesh 
            benesh.c
            beneshp.c
            benesh-dummy.c
            benesh-ekt.c
            benesh-logging.c
            benesh-config.c
            benesh-cohort.c
            benesh-obj.c
            benesh-targets.c
            benesh-tasks.c
            benesh-tpoint.c
            util.c
)

target_link_libraries(benesh PRIVATE ${MPI_C_LIBRARIES})

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

target_include_directories (benesh BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)

if(_apex_FOUND)
    add_definitions(-DUSE_APEX)
    target_link_libraries(benesh PRIVATE apex)
    target_include_directories(benesh BEFORE PRIVATE _apex_INCLUDEDIR)
endif()

set (DEST_DIR "${CMAKE_INSTALL_PREFIX}")
configure_file ("benesh.pc.in" "benesh.pc" @ONLY)

install(TARGETS benesh 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install (FILES "${CMAKE_CURRENT_BINARY_DIR}/benesh.pc"
    DESTINATION "lib/pkgconfig/")

#
# installation stuff (packaging and install commands)
#
write_basic_package_version_file(
    "benesh-config-version.cmake"
    VERSION ${benesh_VERSION}
    COMPATIBILITY AnyNewerVersion)

# generate our config file for find_package()
configure_file (benesh-config.cmake.in benesh-config.cmake @ONLY)

#
# "make install" rules
#
install (TARGETS benesh EXPORT benesh-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
install (EXPORT benesh-targets NAMESPACE benesh::
         DESTINATION ${benesh-pkg}
         FILE "benesh-targets.cmake")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/benesh-config.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/benesh-config-version.cmake"
               "../cmake/xpkg-import.cmake"
         DESTINATION ${benesh-pkg} )
