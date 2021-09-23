cmake_minimum_required(VERSION 3.15)
project(FruStration)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/.cmake-modules/")

set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Og")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Og -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
endif()

#message("CXX debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
#message("CXX release flags: ${CMAKE_CXX_FLAGS_RELEASE}")

if (WIN32)
    set(SDL2_FOUND_TRUE)
    set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libs/windows/sdl2/include")
    set(SDL2_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/libs/windows/sdl2/lib/SDL2.lib")
    set(SDL2MAIN_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/libs/windows/sdl2/lib/SDL2main.lib")
    set(SDL2_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/windows/sdl2/bin/SDL2.dll")
    # avoid name collision with windef.h
    add_definitions(-DNOMINMAX)
else()
    find_package(SDL2 REQUIRED)
endif()

# linker flags
if (APPLE)
    message(FATAL_ERROR "macOS build not yet supported")
    #set(LIBS "-lc++abi -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo")
elseif(UNIX AND NOT APPLE)
    # use groups for older gcc versions
    if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
        set(LIBS "-Wl,--start-group -lGL -ldl")
    else()
        set(LIBS "-lGL -ldl")
    endif()
elseif(WIN32)
	set(LIBS "opengl32.lib ws2_32.lib")
else()
    message(FATAL_ERROR "Unsupported platform")
endif ()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBS}")

# output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_subdirectory(libs)
add_subdirectory(src)