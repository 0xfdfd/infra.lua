cmake_minimum_required(VERSION 3.2)
project(infra)

###############################################################################
# Functions
###############################################################################
function(setup_target_wall name)
    if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif ()
endfunction()

###############################################################################
# Setup library
###############################################################################

add_library(${PROJECT_NAME}
    src/utils/exec.c
    src/utils/list.c
    src/utils/map.c
    src/utils/pipe.c
    src/__init__.c
    src/argparser.c
    src/basename.c
    src/compare.c
    src/cwd.c
    src/dirname.c
    src/dump_any.c
    src/dump_hex.c
    src/execute.c
    src/exepath.c
    src/man.c
    src/map.c
    src/merge_line.c
    src/pairs.c
    src/range.c
    src/readdir.c
    src/readfile.c
    src/split_line.c
    src/strcasecmp.c
    src/writefile.c)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

setup_target_wall(${PROJECT_NAME})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")

###############################################################################
# Dependency
###############################################################################

# Lua
find_package(Lua REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LUA_LIBRARIES})

# System library
if (UNIX)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            m
            rt)
endif ()

###############################################################################
# Install
###############################################################################

install(TARGETS ${PROJECT_NAME}
    EXPORT InfraTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

###############################################################################
# Test
###############################################################################
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(test)
endif()
