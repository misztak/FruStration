
# nativefiledialog
# TODO: use normal folder instead of submodule
set(NFD_SOURCES nativefiledialog/src/nfd_common.c)
if (WIN32)
    list(APPEND NFD_SOURCES nativefiledialog/src/nfd_win.cpp)
    list(APPEND NFD_DEFS UNICODE _UNICODE)
elseif (APPLE)
    list(APPEND NFD_SOURCES nativefiledialog/src/nfd_cocoa.m)
elseif (UNIX AND NOT APPLE AND NOT ANDROID)
    list(APPEND NFD_SOURCES nativefiledialog/src/nfd_gtk.c)
else()
    message(FATAL_ERROR "Unsupported system")
endif()
add_library(nativefiledialog STATIC ${NFD_SOURCES})
target_include_directories(nativefiledialog PUBLIC nativefiledialog/src/include)
target_compile_definitions(nativefiledialog PRIVATE ${NFD_DEFS})
# NFD on linux requires GTK dependency
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(GTK "gtk+-3.0")
        if (GTK_FOUND)
            target_link_libraries(nativefiledialog PRIVATE ${GTK_LIBRARIES})
            add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER})
        else()
            message(FATAL_ERROR "Could not find GTK3 libraries")
        endif()
    endif()
endif()

add_subdirectory(spdlog)

add_subdirectory(simpleini)

add_subdirectory(gl3w)

add_subdirectory(imgui)
