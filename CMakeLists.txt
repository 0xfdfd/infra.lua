cmake_minimum_required(VERSION 3.2)
project(infra)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

function(setup_target_wall name)
    if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif ()
endfunction()

find_package(Lua REQUIRED)
find_package(LibUV REQUIRED)

add_library(${PROJECT_NAME}
    src/utils/list.c
    src/utils/map.c
    src/lua_api.c
    src/lua_errno.c
    src/lua_get_api_info.c
    src/lua_sha256.c
    src/lua_task.c)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${LUA_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${LUA_LIBRARY}
        libuv)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
setup_target_wall(${PROJECT_NAME})

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(test)
endif()
