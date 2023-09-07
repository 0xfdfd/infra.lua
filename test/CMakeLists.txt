
add_executable(infra_test
    case/basename.c
    case/compare.c
    case/dirname.c
    case/dump_any.c
    case/dump_hex.c
    case/man.c
    case/merge_line.c
    case/readdir.c
    case/strcasecmp.c
    cutest.c
    test.c)

target_include_directories(infra_test
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${LUA_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(infra_test
    PRIVATE
        infra
        ${LUA_LIBRARIES})

setup_target_wall(infra_test)

add_test(NAME infra_test
    COMMAND infra_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/case)
