cmake_minimum_required(VERSION 3.2)
project(infra)

function(setup_target_wall name)
    if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif ()
endfunction()

find_package(Lua REQUIRED)

add_library(${PROJECT_NAME}
    src/utils/map.c
    src/__init__.c
    src/basename.c
    src/compare.c
    src/dirname.c
    src/dump_any.c
    src/dump_hex.c
    src/man.c
    src/map.c
    src/merge_line.c
    src/readdir.c
    src/split_line.c
    src/strcasecmp.c)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${LUA_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${LUA_LIBRARIES})

if (UNIX)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            m)
endif ()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
setup_target_wall(${PROJECT_NAME})

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(test)
endif()
