
# modules can include each other (with path from root)
include_directories(.)

if (MSVC)
    # avoid name collisions
    add_definitions(-DNOMINMAX)

    # prevent redefinition errors caused by useless windows.h includes
    add_definitions(-DWIN32_LEAN_AND_MEAN)

    # build with Unicode support
    add_definitions(-DUNICODE -D_UNICODE)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wmissing-declarations
        #Wshadow
        -Wunused

        $<$<CXX_COMPILER_ID:Clang>:-Werror=shadow-uncaptured-local>
        $<$<CXX_COMPILER_ID:Clang>:-Werror=implicit-fallthrough>
        $<$<CXX_COMPILER_ID:Clang>:-Werror=type-limits>
    )

    # make sure file offset size is 64 bits
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR MINGW)
        add_definitions(-D_FILE_OFFSET_BITS=64)
    endif()
endif()

add_subdirectory(core)
add_subdirectory(common)
add_subdirectory(frontend-sdl)
