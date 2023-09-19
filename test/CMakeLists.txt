
add_executable(infra_test
    case/argparser_action.c
    case/argparser_choices.c
    case/argparser_format_help.c
    case/argparser_longopt.c
    case/argparser_narg.c
    case/argparser_shortopt.c
    case/argparser_type.c
    case/basename.c
    case/compare.c
    case/cwd.c
    case/dirname.c
    case/dump_any.c
    case/dump_hex.c
    case/execute.c
    case/exepath.c
    case/man.c
    case/map.c
    case/merge_line.c
    case/pairs.c
    case/range.c
    case/readdir.c
    case/readfile.c
    case/strcasecmp.c
    case/writefile.c
    cutest.c
    test.c)

target_include_directories(infra_test
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${LUA_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(infra_test
    PRIVATE
        infra
        ${LUA_LIBRARIES}
        Threads::Threads)

setup_target_wall(infra_test)

add_test(NAME infra_test
    COMMAND infra_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/case)
