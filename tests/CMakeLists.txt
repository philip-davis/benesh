#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")

add_executable(smoke smoke/smoke.c)

target_include_directories(smoke PRIVATE 
    ${MPI_C_INCLUDE_PATH}
)
target_compile_options(smoke PRIVATE ${MPI_C_COMPILE_FLAGS})
target_link_libraries(smoke ${MPI_C_LIBRARIES} ${MPI_C_LINK_FLAGS} benesh)

#add_executable(bredev redev/util_benchsrBenesh.cpp)

#target_include_directories(bredev PRIVATE
#    ${MPI_CXX_INCLUDE_PATH}
#)
#target_compile_options(bredev PRIVATE ${MPI_CXX_COMPILE_FLAGS})
#target_link_libraries(bredev ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} benesh redev::redev adios2::adios2)

add_executable(bredev_oh omegah/test_twoClientOverlap.cpp)

add_executable(rmbench redev/rmbench.c)

set_property(TARGET bredev_oh PROPERTY CXX_STANDARD 20)

target_include_directories(bredev_oh PRIVATE
    ${MPI_CXX_INCLUDE_PATH}
)
target_compile_options(bredev_oh PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(bredev_oh ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} benesh redev::redev adios2::adios2 Omega_h::omega_h Kokkos::kokkos)

add_executable(proxy_couple wdmcpl/test_proxy_coupling.cpp wdmcpl/test_support.cpp)
set_property(TARGET proxy_couple  PROPERTY CXX_STANDARD 20)
target_include_directories(proxy_couple PRIVATE
    ${MPI_CXX_INCLUDE_PATH}
)
target_compile_options(proxy_couple PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(proxy_couple ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} wdmcpl::wdmcpl redev::redev adios2::adios2 Omega_h::omega_h)

add_executable(xgc_proxy wdmcpl/xgc_proxy.cpp)
target_include_directories(xgc_proxy PRIVATE
    ${MPI_CXX_INCLUDE_PATH}
)
target_compile_options(xgc_proxy PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(xgc_proxy ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} benesh redev::redev adios2::adios2 Omega_h::omega_h Kokkos::kokkos)

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
target_link_libraries(heateq_benesh ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} benesh redev::redev adios2::adios2)
if(_apex_FOUND)
    add_definitions(-DUSE_APEX)
    target_link_libraries(heateq_adhoc apex)
    target_link_libraries(heateq_adhoc_async apex)
    target_link_libraries(heateq_benesh apex)
    target_link_libraries(bredev_oh apex)
    target_link_libraries(xgc_proxy apex)
    target_link_libraries(proxy_couple apex)
    target_link_libraries(rmbench apex)
    set(NEED_APEX "apex")
endif()

install(TARGETS heateq_adhoc heateq_adhoc_async heateq_benesh bredev_oh proxy_couple rmbench
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
