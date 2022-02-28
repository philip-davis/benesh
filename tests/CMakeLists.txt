add_executable(smoke smoke/smoke.c)

target_include_directories(smoke PRIVATE 
    ${MPI_C_INCLUDE_PATH}
)
target_compile_options(smoke PRIVATE ${MPI_C_COMPILE_FLAGS})
target_link_libraries(smoke ${MPI_C_LIBRARIES} ${MPI_C_LINK_FLAGS} benesh)

add_executable(bredev redev/util_benchsrBenesh.cpp)

target_include_directories(bredev PRIVATE
    ${MPI_CXX_INCLUDE_PATH}
)
target_compile_options(bredev PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(bredev ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} benesh redev::redev adios2::adios2)

