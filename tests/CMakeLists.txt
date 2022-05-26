add_executable(smoke smoke/smoke.c)

target_include_directories(smoke PRIVATE 
    ${MPI_C_INCLUDE_PATH}
)
target_compile_options(smoke PRIVATE ${MPI_C_COMPILE_FLAGS})
target_link_libraries(smoke ${MPI_C_LIBRARIES} ${MPI_C_LINK_FLAGS} benesh)

if(_redev_FOUND)
    add_executable(bredev redev/util_benchsrBenesh.cpp)

    target_include_directories(bredev PRIVATE
        ${MPI_CXX_INCLUDE_PATH}
    )
    target_compile_options(bredev PRIVATE ${MPI_CXX_COMPILE_FLAGS})
    target_link_libraries(bredev ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} benesh redev::redev adios2::adios2)
endif()

add_executable(heateq heateq/heateq2d.c heateq/heat.c)
target_include_directories(heateq PRIVATE
    ${MPI_C_INCLUDE_PATH}
)
target_compile_options(heateq PRIVATE ${MPI_C_COMPILE_FLAGS})
target_link_libraries(heateq ${MPI_C_LIBRARIES} ${MPI_C_LINK_FLAGS} m)

add_executable(heateq_adhoc heateq/heateq2d_adhoc.c heateq/heat.c)

target_include_directories(heateq_adhoc PRIVATE
    ${MPI_CXX_INCLUDE_PATH}
)
target_compile_options(heateq_adhoc PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(heateq_adhoc ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} dspaces::dspaces)

add_executable(heateq_adhoc_async heateq/heateq2d_adhoc_async.c heateq/heat.c)

target_include_directories(heateq_adhoc_async PRIVATE
    ${MPI_CXX_INCLUDE_PATH}
)
target_compile_options(heateq_adhoc_async PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(heateq_adhoc_async ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} dspaces::dspaces)

add_executable(heateq_benesh heateq/heateq2d_benesh.c heateq/heat.c)

target_include_directories(heateq_benesh PRIVATE
    ${MPI_CXX_INCLUDE_PATH}
)
target_compile_options(heateq_benesh PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(heateq_benesh ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} benesh)

if(_redev_FOUND)
    add_definitions(-DHAVE_REDEV)
    target_link_libraries(heateq_benesh redev::redev adios2::adios2)
endif()
if(_apex_FOUND)
    add_definitions(-DUSE_APEX)
    target_link_libraries(heateq_adhoc apex)
    target_link_libraries(heateq_adhoc_async apex)
    target_link_libraries(heateq_benesh apex)
    target_link_libraries(heateq apex)
    set(NEED_APEX "apex")
endif()

if(ADIOS2_FOUND)
    message("adding HAVE_ADIOS flag")
    add_definitions(-DHAVE_ADIOS2)
    target_link_libraries(heateq_adhoc adios2::adios2)
    target_link_libraries(heateq_adhoc_async adios2::adios2)
endif()
