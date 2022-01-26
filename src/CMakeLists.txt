add_subdirectory(parser)

add_library(benesh 
            benesh.c
)

target_include_directories(benesh PRIVATE
                           ${MPI_C_INCLUDE_PATH})
target_compile_options(benesh PRIVATE ${MPI_C_COMPILE_FLAGS})
target_link_libraries(benesh
                      ${MPI_C_LIBRARIES} 
                      ${MPI_C_LINK_FLAGS}
                      parser)


