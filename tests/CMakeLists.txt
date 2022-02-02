add_executable(smoke smoke/smoke.c)

target_include_directories(smoke PRIVATE 
    ${MPI_C_INCLUDE_PATH}
)
target_compile_options(smoke PRIVATE ${MPI_C_COMPILE_FLAGS})
target_link_libraries(smoke ${MPI_C_LIBRARIES} ${MPI_C_LINK_FLAGS} benesh)
