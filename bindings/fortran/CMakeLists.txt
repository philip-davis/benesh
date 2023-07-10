set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include(FortranCInterface)

FortranCInterface_HEADER(FC.h MACRO_NAMESPACE "FC_")

add_library(benesh_fortran
    benesh_f2c.c
    benesh_mod.f90
)

set_property(TARGET benesh_fortran PROPERTY EXPORT_NAME fortran)
set_property(TARGET benesh_fortran PROPERTY OUTPUT_NAME benesh_f)

target_include_directories(benesh_fortran
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_Fortran_MODULE_DIRECTORY}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${MPI_CXX_INCLUDE_PATH}
)

target_link_libraries(benesh_fortran
    PRIVATE
        benesh
        ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS}
)

install (TARGETS benesh_fortran EXPORT benesh-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)

install (
    DIRECTORY ${CMAKE_Fortran_MODULE_DIRECTORY}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "benesh*.mod"
        PATTERN "CMakeFiles" EXCLUDE
)
