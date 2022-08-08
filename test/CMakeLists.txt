
add_executable(infra_loader loader.c)

target_link_libraries(infra_loader
    PRIVATE
        infra
        ${LUA_LIBRARY})

setup_target_wall(infra_loader)

set(CASE_LIST
    dump_value_as_string
    equal
    int64
    json
    list
    map
    sha256_once
    strerror
    task)

foreach (item IN LISTS CASE_LIST)
    add_test(NAME ${item}
        COMMAND infra_loader ${CMAKE_CURRENT_SOURCE_DIR}/case/${item}.lua
        WORKING_DIRECTORY $<TARGET_FILE_DIR:infra>)
endforeach()
