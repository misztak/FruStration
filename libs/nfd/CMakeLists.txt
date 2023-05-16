set(SRCS
    src/common.h
    src/nfd_common.h
    src/nfd_common.c
    src/simple_exec.h)

if (WIN32)
    list(APPEND SRCS src/nfd_win.cpp)
    list(APPEND NFD_DEFS UNICODE _UNICODE)
elseif (APPLE)
    list(APPEND SRCS src/nfd_cocoa.m)
else()
    list(APPEND SRCS src/nfd_gtk.c)
endif()

add_library(nfd STATIC ${SRCS})
target_include_directories(nfd PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(nfd INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_definitions(nfd PRIVATE ${NFD_DEFS})

# NFD on linux requires GTK dependency
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(GTK "gtk+-3.0")
        if (GTK_FOUND)
            target_link_libraries(nfd PRIVATE ${GTK_LIBRARIES})
            add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER})
        else()
            message(FATAL_ERROR "Could not find GTK3 libraries")
        endif()
    endif()
endif()
