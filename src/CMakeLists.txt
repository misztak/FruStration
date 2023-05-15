
# modules can include each other (with path from this root (src))
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
        #-Wshadow
        -Wunused

        $<$<CXX_COMPILER_ID:Clang>:-Wshadow-uncaptured-local>
        $<$<CXX_COMPILER_ID:Clang>:-Wimplicit-fallthrough>
        $<$<CXX_COMPILER_ID:Clang>:-Wtype-limits>
    )

    # make sure file offset size is 64 bits
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR MINGW)
        add_definitions(-D_FILE_OFFSET_BITS=64)
    endif()
endif()

# __FILE_NAME__ on gcc is only available since version 12
# so use cmake to manually generate macro definitions for every TU (FILE_BASENAME definition)
# this cannot be used in header files or it would cause an ODR violation
function(define_file_basename_for_sources target)
    get_target_property(source_files "${target}" SOURCES)
    foreach(sourcefile ${source_files})
        # Add the FILE_BASENAME=filename compile definition to the list.
        get_filename_component(basename "${sourcefile}" NAME)
        # Set the updated compile definitions on the source file.
        set_property(
            SOURCE "${sourcefile}" APPEND
            PROPERTY COMPILE_DEFINITIONS "FILE_BASENAME=\"${basename}\"")
    endforeach()
endfunction()

add_subdirectory(core)
add_subdirectory(common)
add_subdirectory(frontend-sdl)
