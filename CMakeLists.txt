# Root cmake file

cmake_minimum_required(VERSION 3.19)

project(FruStration)

set(CMAKE_CXX_STANDARD 17)

# verbose build script output
#set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

# uncomment to generate a compile_commands.json file
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
    # enable asserts in relwithdebinfo builds
    string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
else()
    # same as above
    string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        # use -Og for debug builds
        add_compile_options(-Og)
        # this prevents glibc assertions from being removed by -Og
        add_compile_options(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
    endif()
endif()

# sanity check for default build flags
#message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
#message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
#message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
#message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")

if (WIN32)
    set(SDL2_FOUND_TRUE)
    set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libs/windows/sdl2/include")
    set(SDL2_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/libs/windows/sdl2/lib/SDL2.lib")
    set(SDL2MAIN_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/libs/windows/sdl2/lib/SDL2main.lib")
    set(SDL2_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/windows/sdl2/bin/SDL2.dll")
else()
    find_package(SDL2 REQUIRED)
endif()

# put executable in root of build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_subdirectory(libs)
add_subdirectory(src)
