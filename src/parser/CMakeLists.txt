find_package(BISON)
find_package(FLEX)


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

BISON_TARGET(xcparse xc.y ${CMAKE_CURRENT_BINARY_DIR}/xc_parse.c)
FLEX_TARGET(xclex xc.l ${CMAKE_CURRENT_BINARY_DIR}/xc_lex.c
            COMPILE_FLAGS "-Pxc_ -i"
)
ADD_FLEX_BISON_DEPENDENCY(xclex xcparse)

add_library(parser 
            ${BISON_xcparse_OUTPUTS}
            ${FLEX_xclex_OUTPUTS}
            xc_parse.c
            xc_config.c
)

target_include_directories(parser PUBLIC
                           ${MPI_C_INCLUDE_PATH}
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_BINARY_DIR}
)
set_target_properties(parser PROPERTIES
  OUTPUT_NAME benesh_parser
  C_STANDARD 99
)

install(TARGETS parser
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
